

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
	'timeout_sec_stop_listening_normal': 1.5,
}

default_timeout = 1

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
			[0x92, 'with good password', [0xE0, 0x92, 0x38, 0x00, 0x00, 0x00, 0x00, 0xAA, 0xED]], # exit_mission_boss_mode
			[0x23, 'with good password', [0xE0, 0x23, 0x35, 0xA6, 0x32, 0x18, 0xD3, 0xFF, 0xED]], # reboot

			# set beacon period
			[0x24, '1 minute', [0xE0, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 1, 0xED]],
			[0x24, '3 minutes', [0xE0, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 3, 0xED]],
			[0x24, '100 minutes', [0xE0, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 100, 0xED]],

			# set APRS modes
			[0x0C, 'APRS Mode = Inactive', [0xE0, 0x0C, 0x00, 0x00, 0x00, 0, 0, 0x00, 0xED]],
			[0x0C, 'APRS Mode = Digipeat', [0xE0, 0x0C, 0x00, 0x00, 0x00, 0, 0, 0x01, 0xED]],
			[0x0C, 'APRS Mode = Store&Fwd', [0xE0, 0x0C, 0x00, 0x00, 0x00, 0, 0, 0x02, 0xED]],

			# timestamps
			# datetime.datetime(2023, 9, 10, 21, 12, 29)
			# '0x64fe313d'
			[0x31, '2023-09-10 T21:12:29Z', [0xE0, 0x31, 0x00, 0x00, 0x64, 0xfe, 0x31, 0x3d, 0xED]],

			# request APRS frames
			[0x11, '1 frame', [0xE0, 0x11, 0x00, 0x00, 0, 0, 0, 1, 0xED]],
			[0x11, '2 frames', [0xE0, 0x11, 0x00, 0x00, 0, 0, 0, 2, 0xED]],
			[0x11, '10 frames', [0xE0, 0x11, 0x00, 0x00, 0, 0, 0, 10, 0xED]],
			[0x11, '100 frames', [0xE0, 0x11, 0x00, 0x00, 0, 0, 0, 100, 0xED]],
			[0x11, '255 frames', [0xE0, 0x11, 0x00, 0x00, 0, 0, 0, 255, 0xED]],

			# request experiment data packets
			# [0x11, '1 packet', [0xE0, 0x11, 0x00, 0x00, 0, 0, 0, 1, 0xED]],
			# [0x27, '1 packet', [0xE0, 0x27, 0x00, 0x00, 0, 0, 0, 1, 0xED]],
			# [0x11, '2 packets', [0xE0, 0x11, 0x00, 0x00, 0, 0, 0, 2, 0xED]],
			# [0x27, '2 packets', [0xE0, 0x27, 0x00, 0x00, 0, 0, 0, 2, 0xED]],
			# [0x11, '10 packets', [0xE0, 0x11, 0x00, 0x00, 0, 0, 0, 10, 0xED]],
			# [0x27, '10 packets', [0xE0, 0x27, 0x00, 0x00, 0, 0, 0, 10, 0xED]],
			# [0x11, '100 packets', [0xE0, 0x11, 0x00, 0x00, 0, 0, 0, 100, 0xED]],
			# [0x27, '100 packets', [0xE0, 0x27, 0x00, 0x00, 0, 0, 0, 100, 0xED]],
			# [0x11, '255 packets', [0xE0, 0x11, 0x00, 0x00, 0, 0, 0, 255, 0xED]],
			# [0x27, '255 packets', [0xE0, 0x27, 0x00, 0x00, 0, 0, 0, 255, 0xED]],

			# CCD experiment
			# [0xC0, 'CCD1', [0xE0, 0xC0, 0x00, 0x00, 0, 0, 0, 0x01, 0xED]],
			# [0xC0, 'CCD2', [0xE0, 0xC0, 0x00, 0x00, 0, 0, 0, 0x02, 0xED]],

			# test delay_ms
			[0x93, '1000 ms', [0xE0, 0x93, 0x00, 0x00, 0, 0, 0x03, 0xE8, 0xED]],
			[0x93, '5000 ms', [0xE0, 0x93, 0x00, 0x00, 0, 0, 0x13, 0x88, 0xED]],

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

def fn_run_full_test_sequence(ser: serial.Serial) -> None:
	""" Runs the full test sequence. """

	df = global_store['cmd_df'].copy()

	# filter out the "exit mission boss mode" command, since it breaks all future tests
	df = df[df['cmd_byte_int'] != 0x92]

	for _, row in df.iterrows():
		serial_send_bytes(ser, row['bytes_to_send'],  row['display_name'])
		time.sleep(0.3)
		read_response(ser)

def fn_just_receive_forever(ser: serial.Serial) -> None:
	""" Receives bytes over UART and prints them as ASCII. Loops forever. """
	print(f"Press Ctrl+C to exit the receive loop.")

	while True:
		try:
			data = ser.read()
			if data:
				print(bytes_to_nice_str(data), end='', flush=True)
			
			# TODO: add timestamps between long pauses
		except KeyboardInterrupt:
			print(f"\nKeyboardInterrupt: going back to menu.")
			break

def fn_view_incoming_once(ser: serial.Serial) -> None:
	""" Receives bytes over UART and prints them as ASCII. Runs once."""

	# data = ser.read_all()
	# if data:
	# 	print(bytes_to_nice_str(data), end='', flush=True)
	# else:
	# 	print("No data.")

	read_response(ser)

def fn_test_delay_ms(ser: serial.Serial) -> None:
	""" Tests the delay_ms() function, by requesting that it waits for n seconds, then checking real resp time. """

	# increase timeout for this function
	ser.apply_settings({
		'timeout': 11,
	})

	# values <50ms fail
	for ms_val in [50, 60, 70, 80, 90, 100, 250, 1000, 5000, 10000]:
		byte_6 = ms_val // 256
		byte_7 = ms_val % 256
		serial_send_bytes(ser, [0xE0, 0x93, 0x00, 0x00, 0, 0, byte_6, byte_7, 0xED],  f"0x93 - delay_ms({ms_val}ms)")
		
		time_very_start = time.time()
		bytes_start = ser.read(5)
		time_done_start_read = time.time() # TODO: do something with this time
		bytes_end = ser.read_until(MBOSS_RESPONSE_END_STR)
		end_time = time.time()

		if bytes_start and bytes_end:
			logger.info(f"For delay_ms({ms_val}ms), took {end_time - time_very_start:.3f}s to receive: {bytes_to_nice_str(bytes_start)}{bytes_to_nice_str(bytes_end)}, and {time_done_start_read - time_very_start:.3f}s to start reading.")
		else:
			logger.warning(f"For delay_ms({ms_val}ms), took {end_time - time_very_start:.3f}s to receive: bytes_start={bytes_start}, bytes_end={bytes_end}")

		if end_time - time_very_start > ms_val / 1000 + 0.1:
			logger.warning(f"For delay_ms({ms_val}ms), took longer than expected")

		if f"RESP: delay_duration_ms={ms_val}, starting delay...complete" not in (bytes_start + bytes_end).decode('ascii', errors='ignore'):
			logger.warning(f"For delay_ms({ms_val}ms), did not receive expected response. Maybe a reboot/crash happened?")

	# restore timeout
	ser.apply_settings({
		'timeout': default_timeout,
	})

def fn_restart_emulator(ser: serial.Serial) -> None:
	""" Restarts this emulator. """
	logger.info(f"Restarting emulator.")
	os.execv(sys.executable, ['python'] + sys.argv)

def fn_set_timeout_to_5_sec(ser: serial.Serial) -> None:
	""" Sets the timeout to 5 seconds. """
	global_store['timeout_sec_stop_listening_normal'] = 5.0

def gui_prompt_for_command_and_execute_it(ser: serial.Serial) -> None:
	""" Presents a GUI to the user to select a command to send from the MBOSS. """

	cmd_df = global_store['cmd_df']
	choices_list: list = cmd_df['display_name'].tolist()
	
	function_options = [
		fn_run_full_test_sequence,
		fn_just_receive_forever,
		fn_view_incoming_once,
		fn_test_delay_ms,
		fn_restart_emulator,
		fn_set_timeout_to_5_sec,
	]

	choices_list += [f"--- {f.__name__}() ---" for f in function_options]

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

	if selected_cmd.startswith('---'):
		func_name = selected_cmd.replace('-', '').replace('()', '').strip()
		logger.info(f"Running function: {func_name}()")
		selected_fn = globals()[func_name]
		selected_fn(ser)
		logger.info(f"Done running function: {func_name}()")

	else:
		# lookup that row in the cmd_df
		cmd_info = cmd_df[cmd_df['display_name'] == selected_cmd].iloc[0]

		# send the bytes
		serial_send_bytes(ser, cmd_info['bytes_to_send'],  cmd_info['display_name'])
		read_response(ser)

	# time.sleep(0.3)

def bytes_to_nice_str(byte_obj: bytes, include_tstamp: bool = True) -> str:
	""" Prints a byte object as hex or ASCII, whichever is better.
	Example print: [0xDA][0xBE]INFO: boot complete[0xDA][0xED]
	"""
	out: str = ''
	# byte_obj = byte_obj.replace(MBOSS_RESPONSE_END_STR, MBOSS_RESPONSE_END_STR+b'\n')
	for b in byte_obj:
		# if it's a printable ASCII character
		if 0x20 <= b <= 0x7E:
			out += bytes([b]).decode('ascii')
		else:
			out += f"[{hex(b).upper().replace('0X', '0x')}]"

		if b == MBOSS_RESPONSE_END_STR[-1]:
			if include_tstamp:
				out += f" [<{time.time():,.3f}s>]"

			out += '\n'

	return out

def read_response(ser: serial.Serial) -> None:
	send_finished_time = time.time()

	resp = b''
	last_rx_time = time.time()
	print(f"RX >>", end='', flush=True)
	while time.time() - send_finished_time < 20: # rarely happens, normally breaks (this is for a very very long-running command)
		data = ser.read()
		resp += data
		
		if data:
			print(bytes_to_nice_str(data), end='', flush=True)
			last_rx_time = time.time()

		if time.time() - last_rx_time > global_store['timeout_sec_stop_listening_normal']:
			break

	print(f" [<done @ {time.time():,.3f}s>]")

	if len(resp) >= 200:
		logger.warning(f"Response is too long (>200). len={len(resp)}")
	elif len(resp) == 0:
		logger.warning(f"Response is empty.")
	elif len(resp) > 180:
		logger.warning(f"Response is nearing max len (200 bytes). len={len(resp)}")
	
	
		
def main():
	logger.info(f"Starting main()")

	global_store['cmd_df'] = make_simple_command_table()
	global_store['cmd_df'] = add_to_command_table(global_store['cmd_df'])
	logger.info(f"cmd_df (final):\n{global_store['cmd_df'].to_markdown(index=False)}")

	# write command list to files
	global_store['cmd_df'].to_csv('command_list.csv', index=False)
	global_store['cmd_df'].to_excel('command_list.xlsx', index=False)
	global_store['cmd_df'].to_markdown('command_list.md', index=False)

	logger.info(f"There are {len(global_store['cmd_df'])} commands available, with {len(global_store['cmd_df']['cmd_byte_int'].unique())} distinct command bytes.")

	global_store['port'] = gui_select_serial_port()
	logger.info(f"Selected port: {global_store['port']}")

	with serial.Serial(port=global_store['port'], timeout=default_timeout, baudrate=global_store['baud_rate']) as ser:
		logger.info(f"Opened port successfully.")
		
		while True:
			gui_prompt_for_command_and_execute_it(ser)

			# time.sleep(1) # wait for mboss to send data

			# TODO: pretty-print non-printable chars as hex
			# TODO: continuously check for incoming data while user is clicking buttons


if __name__ == '__main__':
	main()

