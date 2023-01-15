import glob
import importlib.machinery
import importlib.util


def get_hgspy(where: str):
    lib_path = next(glob.iglob(where))
    loader = importlib.machinery.ExtensionFileLoader("hgspy", lib_path)
    spec = importlib.util.spec_from_loader(loader.name, loader)
    hgspy = importlib.util.module_from_spec(spec)
    loader.exec_module(hgspy)

    return hgspy
