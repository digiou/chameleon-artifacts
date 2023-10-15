import os
import requests

from logging import Logger
from pathlib import Path
from typing import Union


class IOConfiguration():
    @property
    def raw_file(self) -> str:
        return self._raw_file

    @raw_file.setter
    def raw_file(self, raw_file_path: str):
        self._raw_file = raw_file_path

    @property
    def small_file(self) -> Union[str,None]:
        return self._small_file

    @small_file.setter
    def small_file(self, small_file_path: str):
        self._small_file = small_file_path

    @property
    def raw_file_downloaded_name(self) -> str:
        return self._raw_file_downloaded_name

    @raw_file_downloaded_name.setter
    def raw_file_downloaded_name(self, raw_file_downloaded_name: str):
        self._raw_file_downloaded_name = raw_file_downloaded_name

    @property
    def results_dir(self) -> str:
        return self._results_dir

    @results_dir.setter
    def results_dir(self, results_dir: str):
        self._results_dir = results_dir

    @property
    def base_path(self) -> str:
        return self._base_path

    @base_path.setter
    def base_path(self, base_path: str):
        self._base_path = base_path

    @property
    def origin(self) -> str:
        return self._origin

    @origin.setter
    def origin(self, origin: str):
        self._origin = origin

    def __init__(self, raw_file: str, small_file: Union[str, None],
                 raw_file_download_name: str, results_dir: str,
                 base_path: str, origin: str):
        self.raw_file = raw_file
        self.small_file = small_file
        self.raw_file_downloaded_name = raw_file_download_name
        self.results_dir = results_dir
        self.base_path = base_path
        self.origin = origin


def download_raw_csv(configuration: IOConfiguration,
                     logger: Logger,
                     skip_download=False, skip_move=True) -> Path:
    final_name = configuration.raw_file
    download_name = configuration.raw_file_downloaded_name
    if not os.path.exists(Path(configuration.base_path)):
        logger.info("Root dir missing, creating...")
        Path(configuration.base_path).mkdir(parents=True, exist_ok=True)
        logger.info("Root dir created!")

    if not os.path.exists(Path(configuration.base_path).joinpath(configuration.results_dir)) or \
            not any(Path(configuration.base_path).joinpath(configuration.results_dir).iterdir()):
        if not os.path.isfile(Path(configuration.base_path).joinpath(final_name)):
            if not os.path.isfile(Path(configuration.base_path).joinpath(download_name)) and not skip_download:
                logger.info(f"Downloading raw CSV file in {Path(configuration.base_path).joinpath(download_name)}...")
                with requests.get(configuration.origin, verify=False, stream=True) as r:
                    r.raise_for_status()
                    with open(Path(configuration.base_path).joinpath(download_name), 'wb') as f:
                        for chunk in r.iter_content(chunk_size=8192):
                            f.write(chunk)
                logger.info("Written raw file to disk!")
            if not skip_move:
                logger.info("Moving downloaded file...")
                Path(configuration.base_path).joinpath(download_name).rename(Path(configuration.base_path).joinpath(final_name))
                logger.info("Moved downloaded file!")
        Path(configuration.base_path).joinpath(configuration.results_dir).mkdir(parents=True, exist_ok=True)
    return Path(configuration.base_path).joinpath(configuration.results_dir)
