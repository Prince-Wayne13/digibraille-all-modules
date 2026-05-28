import glob
import pathlib
root = pathlib.Path.home().joinpath('.platformio', '.cache', 'packages', 'lib')
print(root)
for p in glob.glob(str(root.joinpath('**','*.h')), recursive=True):
    if 'sam' in p.lower() or 'audiotools' in p.lower():
        print(p)
