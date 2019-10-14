"""A setuptools setup module for py_scripting"""

from setuptools import setup, find_packages
from os import path

with open(path.join(path.abspath(path.dirname(__file__)), "README.md")) as f:
  long_description = f.read()

setup(
  name="alog",
  version="1.2.0",
  description="A wrapper around the logging package to provide Alchemy Log functionality",
  long_description=long_description,
  url="https://github.ibm.com/watson-nlu/alog-py",
  author="Gabe Goodhart - IBM Watson",
  author_email="ghart@us.ibm.com",
  license='Copyright IBM 2019 -- All rights reserved.',
  classifiers=[
    'Intended Audience :: Developers',
    'Topic :: Software Development :: User Interfaces',
    'Programming Language :: Python :: 3',
    'Programming Language :: Python :: 3.5',
  ],
  keywords="logging",
  packages=find_packages(),
)
