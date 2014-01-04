import sys

if int(sys.version[0]) >= 3:
    from otama.otama import Otama, __libotama_version__
else:
    from otama import Otama, __libotama_version__
from ._version import __version__
