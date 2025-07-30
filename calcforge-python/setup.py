from setuptools import setup

APP = ['calcforge.py']
DATA_FILES = []
OPTIONS = {
    'argv_emulation': True,
    'iconfile': 'calcforge.icns',  # Use the new valid macOS icon
    'packages': ['PySide6', 'pint', 'requests', 'PIL'],
    'includes': ['PIL', 'PIL.*'],
}

setup(
    app=APP,
    name='CalcForge',
    data_files=DATA_FILES,
    options={'py2app': OPTIONS},
    setup_requires=['py2app'],
)
