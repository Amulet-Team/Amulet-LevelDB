import os
import importlib.util

from shared.generate_pybind_stubs import run


def get_module_path(name: str) -> str:
    spec = importlib.util.find_spec(name)
    assert spec is not None
    module_path = spec.origin
    assert module_path is not None
    return module_path


def get_package_dir(name: str) -> str:
    return os.path.dirname(get_module_path(name))


def main():
    path = get_package_dir("leveldb")
    src_path = os.path.dirname(path)
    run(src_path, "leveldb")


if __name__ == "__main__":
    main()
