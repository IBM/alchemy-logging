"""A setuptools setup module for py_scripting
"""

from setuptools import setup, find_packages
from os import path

with open(path.join(path.abspath(path.dirname(__file__)), "README.md")) as f:
    long_description = f.read()

setup(
    name="alchemy-logging",
    version="1.2.7",
    description="A wrapper around the logging package to provide Alchemy Logging functionality",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/IBM/alchemy-logging",
    author="Gabe Goodhart",
    author_email="ghart@us.ibm.com",
    license="MIT",
    classifiers=[
        "Intended Audience :: Developers",
        "Topic :: Software Development :: User Interfaces",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.5",
        "Programming Language :: Python :: 3.6",
        "Programming Language :: Python :: 3.7",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
    ],
    keywords="logging",
    packages=find_packages(),
)
