try:
    from setuptools import setup, find_packages
except ImportError:
    from distutils.core import setup, find_packages

setup(name="Restpose",
      version="dev",
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
        'Development Status :: 1 - Planning',
        'Environment :: Console',
        'License :: OSI Approved :: BSD License',
        'Programming Language :: Python :: 2.7',
        'Operating System :: Unix',
        ],
      install_requires=[
        'restkit',
        ],
      setup_requires=[
        'nose>=0.11',
        ],
      tests_require=[
        'coverage',
        ],
)
