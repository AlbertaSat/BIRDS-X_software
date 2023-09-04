

import serial # pyserial
import serial.tools
import serial.tools.list_ports

import easygui
from loguru import logger
from dataclasses import dataclass
import polars as pl
import pandas as pd
import re
import time
import sys
import glob
import os

global_store = {
	'baud_rate': 9600,
	'port': None,
	'cmd_df': None,
	'last_cmd_selection_str': None,
}

MBOSS_COMMAND_LENGTH = 9
MBOSS_COMMAND_START_BYTE = 0xE0
MBOSS_COMMAND_END_BYTE = 0xED

# copied from mboss_handler.c # TODO: extract it from that file by reading the file directly


def make_simple_command_table() -> pd.DataFrame:
	# Read from File
	mboss_c_file_path = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'firmware', 'Src', 'mboss_handler.c'))
	with open(mboss_c_file_path, 'r', encoding='utf-8') as f:
		mboss_c_file_contents = f.read()
	logger.info(f"Read mboss_handler.c file contents from path: {mboss_c_file_path}")

	# extract table (between start and end markers)
	hit_start_block = False
	boss_command_table: str = ''
	for line in mboss_c_file_contents.splitlines():
		if 'BossCommandEntry boss_command_table[]' in line:
			hit_start_block = True
			continue
		if hit_start_block:
			if '};' in line:
				break
			boss_command_table += line.strip() + '\n'
	logger.info(f"boss_command_table (from .c file):\n{boss_command_table}")

	# Do Transformation
	cmd_regex = re.compile(r"\{(?P<cmd_byte>0x[0-9A-F]{2}), (?P<cmd_func>[a-zA-Z0-9_]+)\}", re.IGNORECASE)
	
	cmd_list = re.findall(cmd_regex, boss_command_table)

	cmd_df = pd.DataFrame(cmd_list, columns=['cmd_byte_str', 'cmd_func'])

	cmd_df['cmd_byte_int'] = cmd_df['cmd_byte_str'].apply(lambda x: int(x[2:], 16))
		
	# polars version
	# boss_command_df = boss_command_df.with_columns(
	# 	bytes_to_send = pl.col('cmd_byte').apply(lambda x: bytes([MBOSS_COMMAND_START_BYTE, x] + [0] * (MBOSS_COMMAND_LENGTH - 3) + [MBOSS_COMMAND_END_BYTE])),
	# 	display_name = pl.struct(['cmd_byte, cmd_func']).apply(lambda x: f"{x.cmd_byte:02X} - {x.cmd_func}")
	# )
	cmd_df['bytes_to_send'] = cmd_df['cmd_byte_int'].apply(lambda x: list([MBOSS_COMMAND_START_BYTE, x] + [0] * (MBOSS_COMMAND_LENGTH - 3) + [MBOSS_COMMAND_END_BYTE]))
	cmd_df['display_name'] = cmd_df.apply(lambda x: f"0x{x.cmd_byte_int:02X} - {x.cmd_func}", axis=1)

	logger.info(f"cmd_df (as loaded from .c file):\n{cmd_df.to_markdown()}")

	return cmd_df

def add_to_command_table(cmd_df: pd.DataFrame) -> pd.DataFrame:
	""" Adds cases with passwords and special byte args. """

	cmd_df = cmd_df.copy()

	overwrite_df = pd.DataFrame(
		[
			[0x22, 'with good password', [0xE0, 0x22, 0x38, 0x00, 0x00, 0x00, 0x00, 0xAA, 0xED]], # exit_mission_boss_mode
			[0x19, 'with good password', [0xE0, 0x19, 0x35, 0xA6, 0x32, 0x18, 0xD3, 0xFF, 0xED]], # reboot
			[0x21, 'with good password', [0xE0, 0x21, 0x37, 0x56, 0xCD, 0x21, 0x3D, 0xEE, 0xED]], # clear flash
		],
		columns=['cmd_byte_int', 'desc', 'bytes_to_send']
	)

	overwrite_df = overwrite_df.merge(cmd_df, on='cmd_byte_int', how='left', suffixes=('', '_original'))
	overwrite_df['display_name'] = overwrite_df.apply(lambda x: f"0x{x.cmd_byte_int:02X} - {x.cmd_func} ({x.desc})", axis=1)
	overwrite_df = overwrite_df.rename(columns={'cmd_func_original': 'cmd_func', 'cmd_byte_str_original': 'cmd_byte_str'})

	cmd_df = pd.concat([
		cmd_df,
		overwrite_df
	], ignore_index=True)

	cmd_df = cmd_df.sort_values(by='cmd_byte_int')
	cmd_df = cmd_df[['bytes_to_send', 'display_name', 'cmd_func', 'cmd_byte_int', 'cmd_byte_str', 'desc']]

	return cmd_df

def gui_select_serial_port() -> str:
	""" Presents a GUI to the user to select a serial port. """
	# Get a list of available serial ports
	available_ports = serial.tools.list_ports.comports()
	# FIXME: confirm non-Windows behavior
	
	# Create a list of port names to display in the GUI
	port_names = [port.device for port in available_ports]
	
	# Show a selection dialog to the user
	selected_port = easygui.choicebox("Select a Serial Port:", "Serial Port Selection", port_names)

	if not selected_port:
		logger.info(f"User canceled port selection")
		sys.exit(0)
	
	return selected_port

def serial_send_bytes(ser: serial.Serial, bytes_to_send: bytes|list[int], display_name: str) -> None:
	if isinstance(bytes_to_send, list):
		bytes_to_send = bytes(bytes_to_send)

	# logger.info(f"Sending bytes: >>{bytes_to_send}")
	logger.info(f"Sending '{display_name}': >>{bytes_to_send.hex(' ').upper()}")
	ser.write(bytes_to_send)
	# logger.info(f"Sent    bytes: >>{bytes_to_send}")
	
def gui_prompt_for_command_and_execute_it(ser: serial.Serial) -> None:
	""" Presents a GUI to the user to select a command to send from the MBOSS. """

	cmd_df = global_store['cmd_df']
	choices_list: list = cmd_df['display_name'].tolist()

	preselect_int = 0
	if global_store['last_cmd_selection_str']:
		preselect_int = choices_list.index(global_store['last_cmd_selection_str'])
	
	selected_cmd: str = easygui.choicebox(
		msg="Select a command to issue",
		choices=choices_list,
		preselect=preselect_int,
	)

	if not selected_cmd:
		logger.info(f"User canceled command selection")
		sys.exit(0)

	global_store['last_cmd_selection_str'] = selected_cmd

	# lookup that row in the cmd_df
	cmd_info = cmd_df[cmd_df['display_name'] == selected_cmd].iloc[0]

	# send the bytes
	serial_send_bytes(ser, cmd_info['bytes_to_send'], cmd_info['display_name'])

	time.sleep(0.3)

def main():
	logger.info(f"Starting main()")

	global_store['cmd_df'] = make_simple_command_table()
	global_store['cmd_df'] = add_to_command_table(global_store['cmd_df'])
	logger.info(f"cmd_df (final):\n{global_store['cmd_df'].to_markdown()}")

	global_store['port'] = gui_select_serial_port()
	logger.info(f"Selected port: {global_store['port']}")

	with serial.Serial(port=global_store['port'], timeout=1, baudrate=global_store['baud_rate']) as ser:

		while True:
			gui_prompt_for_command_and_execute_it(ser)

			# read response
			resp = ser.read(10000)
			logger.info(f"Received response: len={len(resp)}")
			print(f"RX >>{resp}")

			# TODO: pretty-print non-printable chars as hex
			# TODO: continuously check for incoming data while user is clicking buttons


if __name__ == '__main__':
	main()

