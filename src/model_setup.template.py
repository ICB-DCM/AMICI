import setuptools.command.install
import shutil
from distutils.sysconfig import get_python_lib


class CompiledLibInstall(setuptools.command.install.install):
    """
    Specialized install to install to python libs
    """

    def run(self):
        """
        Run method called by setup
        :return:
        """
        # Get filenames from CMake variable
        filenames = '${PYTHON_INSTALL_FILES}'.split(';')

        # Directory to install to
        install_dir = get_python_lib()

        # Install files
        [shutil.copy(filename, install_dir) for filename in filenames]


if __name__ == '__main__':
    setuptools.setup(
        name='${PROJECT_NAME}',
        packages=['${PROJECT_NAME}'],
        cmdclass={'install': CompiledLibInstall}
    )
    
