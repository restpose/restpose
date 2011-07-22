try:
    from setuptools import setup, find_packages
except ImportError:
    from distutils.core import setup, find_packages
import restpose.version

setup(name="Restpose",
      version=restpose.version.__version__,
      packages=find_packages(),
      include_package_data=True,
      author='Richard Boulton',
      author_email='richard@tartarus.org',
      description='Client for the Restpose Server',
      long_description=__doc__,
      zip_safe=False,
      platforms='any',
      license='MIT',
      url='https://github.com/rboulton/restpose',
      classifiers=[
        'Development Status :: 2 - Pre-Alpha',
        'Environment :: Console',
        'License :: OSI Approved :: MIT License',
        'Programming Language :: Python :: 2.7',
        'Operating System :: Unix',
      ],
      install_requires=[
        'restkit>=3.2.3',
      ],
      setup_requires=[
        'nose>=0.11',
      ],
      tests_require=[
        'coverage',
      ],
)
