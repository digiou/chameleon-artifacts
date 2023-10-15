import psutil


def get_physical_cores() -> int:
    ret = psutil.cpu_count(logical=False)
    if ret is None:
        raise EnvironmentError
    return ret
