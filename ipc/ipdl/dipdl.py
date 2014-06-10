# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

import optparse, os, re, sys
from cStringIO import StringIO
import itertools

from ipdl.dgen import IPDLDocGen
import ipdl
import json

# process command line
op = optparse.OptionParser(usage='dipdl.py [options] IPDLfiles...')
op.add_option('-I', '--include', dest='includedirs', default=[],
              action='append',
              help='Additional directory to search for included protocol specifications')
op.add_option('-v', '--verbose', dest='verbosity', default=1, action='count',
              help='Verbose output (specify -v or -vv for more detailed output)')
op.add_option('-o', '--output', dest='dest', default="", help="Store output in this file (stdout is default)")
op.add_option('-p', dest='pparse', default=0, action='count', help="Add this flag to parse protocols")
op.add_option('-t', dest='tparse', default=0, action='count', help="Add this flag to parse types (unions, structs)")
op.add_option('-r', '--resolve', dest='resolve', default="", help="Parse protocols and resolve parameter types "+
  "specified in this file (output from -t option)")


options, files = op.parse_args()
verbosity = options.verbosity
pparse = options.pparse
tparse = options.tparse
dest = options.dest
resolve = options.resolve

includedirs = [ os.path.abspath(incdir) for incdir in options.includedirs ]

if pparse == 0 and tparse == 0:
  op.error("Please use either -t or -p")

if not len(files):
  op.error("No IPDL files specified")

if not os.path.isfile(resolve) and len(resolve) > 0:
  op.error("Types file " + resolve + " doesn't exist")

if os.path.isfile(dest):
  op.error("Output file "+dest+" already exists.")

try:
  if not dest == "":
    out = open(dest, 'w')
  else:
    out = sys.stdout
except IOError:
  op.error("Can't open file "+dest+" for writing.")

data = ""

try:
  if not resolve == "":
    res = open(resolve)
    data = res.read()
except IOError:
  op.error("Can't open file "+resolve)

def normalizedFilename(f):
    if f == '-':
        return '<stdin>'
    return f

protocols = []

if not data == "":
  types = json.loads(data)

  # if we parse types we have a special verbosity for it
  verbosity = -1
else:
  types = []

# parse and type-check all protocols & generate db
for f in files:
  filename = normalizedFilename(f)
  if f == '-':
    fd = sys.stdin
  else:
    fd = open(f)

  specstring = fd.read()
  fd.close()

  ast = ipdl.parse(specstring, filename, includedirs=includedirs)
  if ast is None:
    print >>sys.stderr, 'Specification could not be parsed.'
    sys.exit(1)

  # generate the catalog
  ast = ipdl.parse(None, filename, includedirs=includedirs)

  IPDLDocGen().dgen(ast, pparse=pparse, tparse=tparse, protocols=protocols, types=types, verbosity=verbosity)

final = {}
if len(protocols): final["protocols"] = protocols

if not verbosity == -1:
  if len(types): final["types"] = types

out.write(json.dumps(final, indent=4))
