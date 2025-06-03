from setuptools import setup

APP = ['calcforge.py']
DATA_FILES = []
OPTIONS = {
    'argv_emulation': True,
    'iconfile': 'calcforge.ico',  # Use your icon if available, else remove this line
    'packages': ['PySide6', 'pint', 'requests', 'Pillow'],
}

setup(
    app=APP,
    name='CalcForge',
    data_files=DATA_FILES,
    options={'py2app': OPTIONS},
    setup_requires=['py2app'],
)
