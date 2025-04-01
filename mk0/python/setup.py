from setuptools import setup

setup(
    name="deloop_mk0",
    version="0.1.0",
    packages=["deloop_mk0"],
    install_requires=[
        "pyserial",
        "protobuf",
        "grpcio-tools",
        "cmd2",
        "tabulate",
    ],
    package_data={"deloop_mk0": ["**/*.json"]},
    py_modules=["log_pb2", "stream_pb2"],
    entry_points={
        "console_scripts": [
            "deloop_mk0_repl = deloop_mk0.repl:main",
        ],
    },
)
