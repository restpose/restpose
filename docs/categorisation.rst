Categorisation
==============

RestPose currently offers a very straightforward categorisation system, aimed
mainly at performing language guessing (though it would be easy to plug more
advanced algorithms into the system in future).  The system requires a sample
of text for each target category, and generates an ordered list of ngrams which
are most significant in that sample.

The algorithm used is the same as that used by "libtextcat", and based on a
1994 paper entitled "N-gram-based text categorization" by William B. Cavnar and
John M. Trenkle.  It is a very simple algorithm, but processes text extremely
fast and performs the language guessing text extremely well (given suitable
training data).  At the time of writing, the paper can be downloaded from
http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.21.3248&rep=rep1&type=pdf

Training
========

To train a model, you need some sample data.  One approach for language
guessing is to download some sample data from wikipedia, which can be done
manually, or can be done using the script in
`scripts/get_wikipedia_sample_text.py`.  This script will download 512k of text
from random wikipedia pages in each language supported by RestPose, and store
it in a directory named `lang_samples`.  This sample data can then be used to
train the model.

Once you have sample data, the "restpose" program can be used on the command
line to build a model, producing a JSON description of the model on stdout.
This can be quite long, so you'll probably want to redirect the output to a
file.  For example::

  ./restpose -a train -d lang_samples -l en -l fr

The above command will train a model using the sample data for english and
french.  To add each language, add a "-l lang" parameter, where lang is the
language code for the language to add.

For convenience, the command to train a model using all the data downloaded by
the `scripts/get_wikipedia_sample_text.py` script is::

  ./restpose -a train -d lang_samples -l da -l de -l en -l es -l fi -l fr -l hu -l it -l ja -l ko -l nl -l no -l pt -l ro -l ru -l sv -l tr -l zh
