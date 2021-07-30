import os
import logging

REPORT_DIR = os.path.join(os.getcwd(), 'reports')

# logger
logger = logging.getLogger(__name__)
logger.setLevel(level=logging.DEBUG)
formatter = logging.Formatter('%(asctime)s - %(levelname)s - %(message)s')
handler_console = logging.StreamHandler()
handler_console.setLevel(logging.DEBUG)
logger.addHandler(handler_console)

file_handler = logging.FileHandler(os.path.join(REPORT_DIR, 'report.log'))
file_handler.setLevel(logging.DEBUG)
file_handler.setFormatter(formatter)
logger.addHandler(file_handler)
