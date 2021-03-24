#!/usr/bin/python3


""" This script gives you the ability to easily send a sequence of T=1 blocks to a smartcard using the bridge. See end of file. """

import time
import attr
import serial



class BridgeConnector:
	CTRL_BYTE_DATA = b'\x00'
	CTRL_BYTE_ACK = b'\x05'
	LEN_FIELD_SIZE = 3   # Size in number of bytes
	CTRL_FIELD_SIZE = 1
	CHECK_SIZE = 1
	
	def __init__(self, serial_con, logger):
		self._serial_con = serial_con
		self._logger = logger
		self.timeout = 1
	
	def send_data(self, data):
		
		if data == b'':
			ctrl_byte = 0x02
			check_byte = 0x00
			bytes_to_send = bytes([ctrl_byte, check_byte])
		else:
			# Preparing and sending the data block ...
			data_len = len(data)
			ctrl_byte = 0x00
			len_byte1, _ = divmod(data_len, (2 ** 16))
			len_byte2, len_byte3 = divmod(data_len, (2 ** 8))

			bytes_to_send1 = bytes([ctrl_byte, len_byte1, len_byte2, len_byte3])
			bytes_to_send2 = data
			bytes_to_send3 = bytes([0x00])
			bytes_to_send = bytes_to_send1 + bytes_to_send2 + bytes_to_send3

		bytes_sent = 0
		while bytes_sent < len(bytes_to_send):
			bytes_sent_this_round = self._serial_con.write(bytes_to_send[bytes_sent:])
			if bytes_sent_this_round is not None:
				bytes_sent += bytes_sent_this_round

		# Flushing the buffer to make sure that those bytes are actually sent and not remaining in the buffer.
		self._serial_con.flush()

		# Waiting for the ACK ...
		r = self._serial_con.read(1)
		while r != b'\x05':
			r = self._serial_con.read(1)

		r = self._serial_con.read(1)
		if r != b'\x00':
			raise Exception("ACK Problem.")
		
		self._logger.log_send(data)
		
		return bytes_sent
	
	def send_reset(self):
		self.send_data(b'')
		
		self._logger.log_info("Card reseted.")
		
	def receive(self):
		start_time = time.time()

		ctrl_byte = self._recv_control_field(start_time + self.timeout - time.time())

		if ctrl_byte == self.CTRL_BYTE_DATA:
			data_len = self._recv_len_field(start_time + self.timeout - time.time())
			data = self._recv_data_field(data_len, start_time + self.timeout - time.time())

		check_byte = self._recv_check_field(start_time + self.timeout - time.time())
		self._check_block_integrity(check_byte, data_len, data)

		self._send_ack()
		
		self._logger.log_receive(data)
		
		return data
		
		
	def _send_ack(self):
		self._serial_con.write(b'\x05\x00')

	def _recv_control_field(self, timeout_left):
		"""
		This function receives the byte coding for the type of block being transmitted by the bridge.

		:param timeout_left: Amount of time left to receive the len field.
		:return: The control byte of the block being received from the bridge.
		"""
		self._serial_con.timeout = timeout_left
		data = self._serial_con.read(self.CTRL_FIELD_SIZE)
		nb_bytes_read = len(data)

		if nb_bytes_read != self.CTRL_FIELD_SIZE:
			raise SmartcardCtrlFieldError()

		return data

	def _recv_len_field(self, timeout_left):
		"""
		This function receives the three bytes coding the size of the data field transmitted by the bridge.
		It then converts them into an exploitable integer value

		:param timeout_left: Amount of time left to receive the len field.

		:return: Size (in number of bytes) of the information field transmitted by the bridge.
		"""

		self._serial_con.timeout = timeout_left
		data = self._serial_con.read(self.LEN_FIELD_SIZE)
		nb_bytes_read = len(data)

		if nb_bytes_read != self.LEN_FIELD_SIZE:
			raise SmartcardLenFieldError()

		decoded_length = int.from_bytes(data, "big")

		return decoded_length

	def _recv_data_field(self, size, timeout_left):
		"""
		This function is aimed to receive the data payload contained in the block currently being transmitted by the bridge.

		:param timeout_left: Amount of time left to receive the len field.
		:param size: Number of bytes expected to be received in the data section of the block.
		:return: Bytes of the payload.
		"""

		self._serial_con.timeout = timeout_left
		data = self._serial_con.read(size)
		nb_bytes_read = len(data)

		if nb_bytes_read != size:
			raise SmartcardInfFieldError()

		return data


	def _recv_check_field(self, timeout_left):
		"""
		This function receives the checksum.

		:param timeout_left: Amount of time left to receive the len field.
		:return: The checksum of the block being received from the bridge.
		"""
		self._serial_con.timeout = timeout_left
		data = self._serial_con.read(self.CHECK_SIZE)
		nb_bytes_read = len(data)

		if nb_bytes_read != self.CTRL_FIELD_SIZE:
			raise SmartcardCheckFieldError()

		return data

	def _check_block_integrity(self, check_byte, data_size, data):
		if check_byte != b'\x00':
			raise SmartcardChecksumError()
		
	def execute_all_requests(self, requests):
		for r in requests:
			time.sleep(0.2)
			self._logger.log_info("#########################   Sending request ...   ###############################")
			self.send_data(r)
			self.receive()
		


class Logger:
	
	def log_info(self, msg):
		print("[INFO] " + msg)
		
	def log_send(self, msg):
		print("[SENT] " + hex_to_hexstr(msg))
		
	def log_receive(self, msg):
		print("[RECV] " + hex_to_hexstr(msg))
	
	def log_error(self, msg):
		print("[ERR] " + msg)





@attr.s
class SmartcardError(Exception):
	message = attr.ib(type=str, default="")
	pass


class SmartcardTimeoutError(SmartcardError):
	pass


class SmartcardLenFieldError(SmartcardTimeoutError):
	pass


class SmartcardCtrlFieldError(SmartcardTimeoutError):
	pass


class SmartcardCheckFieldError(SmartcardTimeoutError):
	pass


class SmartcardInfFieldError(SmartcardTimeoutError):
	pass


class SmartcardChecksumError(SmartcardError):
	pass



# This function has been taken from the Boofuzz project (https://github.com/jtpereyda/boofuzz)
def hex_to_hexstr(input_bytes):
	"""
	Render input_bytes as ASCII-encoded hex bytes, followed by a best effort
	utf-8 rendering.

	Args:
		input_bytes (bytes): Arbitrary bytes

	Returns:
		str: Printable string
	"""
	return hex_str(input_bytes)# + " " + repr(input_bytes)
	
	
# This function has been taken from the Boofuzz project (https://github.com/jtpereyda/boofuzz)
def hex_str(s):
	"""
	Returns a hex-formatted string based on s.

	Args:
		s (bytes): Some string.

	Returns:
		str: Hex-formatted string representing s.
	"""
	return " ".join("{:02x}".format(b) for b in bytearray(s))


def lrc(data):
    s = 0

    for byte in data:
        s = s ^ byte

    return bytes([s])




s = serial.Serial(port="/dev/ttyUSB0", baudrate=9600)



requests = []
requests.append(b'\x00@\x06\x00\x124V\x01\xca\xfd')
requests.append(b'\x00\x90\x00\x90')

l = Logger()
b = BridgeConnector(s, l)


b.send_reset()
b.execute_all_requests(requests)



