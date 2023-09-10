

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
MBOSS_RESPONSE_END_STR = b"\xDA\xED"

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
	cmd_df['bytes_to_send'] = cmd_df['cmd_byte_int'].apply(lambda x: list([MBOSS_COMMAND_START_BYTE, x] + [0] * (MBOSS_COMMAND_LENGTH - 3) + [MBOSS_COMMAND_END_BYTE]))
	cmd_df['display_name'] = cmd_df.apply(lambda x: f"0x{x.cmd_byte_int:02X} - {x.cmd_func}", axis=1)

	logger.info(f"cmd_df (as loaded from .c file):\n{cmd_df.to_markdown(index=False)}")

	return cmd_df

def add_to_command_table(cmd_df: pd.DataFrame) -> pd.DataFrame:
	""" Adds cases with passwords and special byte args. """

	cmd_df = cmd_df.copy()

	overwrite_df = pd.DataFrame(
		[
			[0x22, 'with good password', [0xE0, 0x22, 0x38, 0x00, 0x00, 0x00, 0x00, 0xAA, 0xED]], # exit_mission_boss_mode
			[0x19, 'with good password', [0xE0, 0x19, 0x35, 0xA6, 0x32, 0x18, 0xD3, 0xFF, 0xED]], # reboot
			[0x21, 'with good password', [0xE0, 0x21, 0x37, 0x56, 0xCD, 0x21, 0x3D, 0xEE, 0xED]], # clear flash

			# set beacon period
			[0x20, '1 minute', [0xE0, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 1, 0xED]],
			[0x20, '3 minutes', [0xE0, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 3, 0xED]],
			[0x20, '100 minutes', [0xE0, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 100, 0xED]],

			# set APRS modes
			[0x01, 'APRS Mode = Inactive', [0xE0, 0x01, 0x00, 0x00, 0x00, 0, 0, 0x00, 0xED]],
			[0x01, 'APRS Mode = Digipeat', [0xE0, 0x01, 0x00, 0x00, 0x00, 0, 0, 0x01, 0xED]],
			[0x01, 'APRS Mode = Store&Fwd', [0xE0, 0x01, 0x00, 0x00, 0x00, 0, 0, 0x02, 0xED]],

			# timestamps
			# datetime.datetime(2023, 9, 10, 21, 12, 29)
			# '0x64fe313d'
			[0x16, '2023-09-10 T21:12:29Z', [0xE0, 0x16, 0x00, 0x00, 0x64, 0xfe, 0x31, 0x3d, 0xED]],
			[0x17, '2023-09-10 T21:12:29Z', [0xE0, 0x16, 0x00, 0x00, 0x64, 0xfe, 0x31, 0x3d, 0xED]],

			# request APRS frames
			[0x02, '1 frame', [0xE0, 0x02, 0x00, 0x00, 0, 0, 0, 1, 0xED]],
			[0x02, '2 frames', [0xE0, 0x02, 0x00, 0x00, 0, 0, 0, 2, 0xED]],
			[0x02, '10 frames', [0xE0, 0x02, 0x00, 0x00, 0, 0, 0, 10, 0xED]],
			[0x02, '100 frames', [0xE0, 0x02, 0x00, 0x00, 0, 0, 0, 100, 0xED]],
			[0x02, '255 frames', [0xE0, 0x02, 0x00, 0x00, 0, 0, 0, 255, 0xED]],
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

	cmd_df = cmd_df.sort_values(by='cmd_byte_int', kind='stable')
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
	serial_send_bytes(ser, cmd_info['bytes_to_send'],  cmd_info['display_name'])

	# time.sleep(0.3)

def main():
	logger.info(f"Starting main()")

	global_store['cmd_df'] = make_simple_command_table()
	global_store['cmd_df'] = add_to_command_table(global_store['cmd_df'])
	logger.info(f"cmd_df (final):\n{global_store['cmd_df'].to_markdown(index=False)}")

	# write command list to files
	global_store['cmd_df'].to_csv('command_list.csv', index=False)
	global_store['cmd_df'].to_excel('command_list.xlsx', index=False)
	global_store['cmd_df'].to_markdown('command_list.md', index=False)

	global_store['port'] = gui_select_serial_port()
	logger.info(f"Selected port: {global_store['port']}")

	with serial.Serial(port=global_store['port'], timeout=3, baudrate=global_store['baud_rate']) as ser:
		logger.info(f"Opened port successfully.")
		
		while True:
			gui_prompt_for_command_and_execute_it(ser)
			send_finished_time = time.time()

			# time.sleep(1) # wait for mboss to send data

			# read response
			resp = ser.read_until(MBOSS_RESPONSE_END_STR, size=10000)
			resp_finished_time = time.time()
			logger.info(f"Received response: len={len(resp)}, time={resp_finished_time - send_finished_time:.3f}s")
			print(f"RX >>{resp}")

			# time.sleep(2) # wait more and read again, in case there's more
			resp2 = ser.read(10000)
			if len(resp2) > 0:
				logger.info(f"More data available: len={len(resp2)}")
				print(f"RX >>{resp2}")

			# TODO: pretty-print non-printable chars as hex
			# TODO: continuously check for incoming data while user is clicking buttons


if __name__ == '__main__':
	main()

