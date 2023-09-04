

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
boss_command_table = """
	{0xFF, boss_cmd_turn_off_payload},
	{0x0E, boss_cmd_set_active_aprs_mode},
	{0x12, boss_cmd_transfer_data_packets},
	{0x03, boss_cmd_send_temperature},
	{0x04, boss_cmd_enable_pin_diode_experiment},
	{0x05, boss_cmd_disable_pin_diode_experiment},
	{0x06, boss_cmd_enable_radfet_experiment},
	{0x07, boss_cmd_disable_radfet_experiment},
	{0x08, boss_cmd_enable_both_experiments},
	{0x09, boss_cmd_disable_both_experiments},
	{0x10, boss_cmd_echo_command},
	{0x13, boss_cmd_set_pin_diode_polling_time},
	{0x14, boss_cmd_set_radfet_polling_time},
	{0x15, boss_cmd_set_both_polling_time},
	{0x16, boss_cmd_set_unix_timestamp},
	{0x17, boss_cmd_set_unix_timestamp_shutdown},
	{0x18, boss_cmd_run_power_on_self_test},
	{0x19, boss_cmd_force_reboot_system},
	{0x20, boss_cmd_set_beacon_period},
	{0x21, boss_cmd_clear_flash_memory},
	{0x22, boss_cmd_exit_mission_boss_mode},
	{0x23, boss_cmd_get_sys_uptime_and_reboot_reason},
	{0x24, boss_cmd_get_unix_timestamp}
"""

def make_simple_command_table() -> pd.DataFrame:
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

	logger.info(f"cmd_df:\n{cmd_df.to_markdown()}")

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

