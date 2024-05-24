#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright 2024 - The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the',  help='License');
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an',  help='AS IS' BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
import sys
import urllib.request
import hashlib


def download_resource(sha1_file, output_file):
    """Downloads a resource from Google Storage based on its SHA1 hash.

    This function reads the SHA1 hash from the given file, constructs the
    corresponding URL on Google Storage, and downloads the resource in a
    streaming fashion to conserve memory for large files. It also verifies the
    downloaded data against the expected SHA1 hash to ensure integrity.

    Args:
        sha1_file (str): The path to a file containing the SHA1 hash of the resource.
        output_file (str): The path where the downloaded resource should be saved.

    Raises:
        ValueError: If the downloaded data does not match the expected SHA1 hash.
        urllib.error.URLError: If there's an issue downloading the resource.
        FileNotFoundError: If the sha1_file is not found.
    """

    with open(sha1_file, "r") as f:
        expected_sha1 = f.read().strip()

    # Our interpreter does not have https.
    url = f"http://storage.googleapis.com/chromium-webrtc-resources/{expected_sha1}"
    sha1 = hashlib.sha1()  # Initialize the SHA1 hash object

    with urllib.request.urlopen(url) as response, open(output_file, "wb") as out_file:
        while True:
            chunk = response.read(8192)  # Read in chunks of 8KB
            if not chunk:
                break
            out_file.write(chunk)
            sha1.update(chunk)  # Update the SHA1 hash with the downloaded chunk

    downloaded_sha1 = sha1.hexdigest()
    if downloaded_sha1 != expected_sha1:
        raise ValueError("Downloaded data does not match expected SHA1 hash!")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: download_resource.py <sha1_file> <output_file>")
        sys.exit(1)
    download_resource(sys.argv[1], sys.argv[2])
