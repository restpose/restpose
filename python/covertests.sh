# Run tests with condition coverage.
# Requires the python package "instrumental" to be installed.

instrumental -rs -t restpose `which nosetests`
