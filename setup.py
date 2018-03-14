"""A setuptools setup module for py_scripting"""

from setuptools import setup, find_packages
from os import path

with open(path.join(path.abspath(path.dirname(__file__)), "README.md")) as f:
  long_description = f.read()

setup(
  name="alog",
  version="1.0.0",
  description="A wrapper around the logging package to provide Alchemy Log functionality",
  long_description=long_description,
  url="https://github.ibm.com/watson-nlu/alog-py",
  author="Gabe Hart - IBM Watson",
  author_email="ghart@us.ibm.com",
  license=None, # Not sure what should go here since it's IBM code
  classifiers=[
    'Development Status :: 4 - Beta',
    'Intended Audience :: Developers',
    'Topic :: Software Development :: User Interfaces',
    'Programming Language :: Python :: 2',
    'Programming Language :: Python :: 2.7',
    'Programming Language :: Python :: 3',
    'Programming Language :: Python :: 3.5',
  ],
  keywords="logging",
  packages=find_packages(),
)
