import logging
import sys

from logging import Logger


def prepare_logger(user_log_level='info') -> Logger:
    logger = logging.getLogger()
    numeric_level = getattr(logging, user_log_level.upper(), None)
    if not isinstance(numeric_level, int):
        raise ValueError(f"Invalid log level: {user_log_level}")
    logger.setLevel(numeric_level)
    handler = logging.StreamHandler(sys.stdout)
    handler.setLevel(numeric_level)
    formatter = logging.Formatter('[%(asctime)s] %(message)s')
    handler.setFormatter(formatter)
    logger.addHandler(handler)
    return logger
