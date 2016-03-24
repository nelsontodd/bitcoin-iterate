from os.path        import join, dirname, abspath
from distutils.core import setup
from Cython.Build   import cythonize

ITERATE_ROOT = abspath(join(dirname(abspath(__file__)), ".."))

setup(
    name = "bitcoin-iterate",
    ext_modules = cythonize("iterate.pyx",
                            sources=[join(ITERATE_ROOT, 'types.c')],
                            include_path=[ITERATE_ROOT, join(ITERATE_ROOT, 'ccan')])
)

