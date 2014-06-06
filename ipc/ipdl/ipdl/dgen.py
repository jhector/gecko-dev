import os, sys

from ipdl.ast import Visitor
from ipdl.ast import IN, OUT, INOUT, ASYNC, SYNC, INTR
from ipdl.cgen import CodePrinter

import ipdl

class IPDLDocGen(CodePrinter, Visitor):
	def __init__(self, outf=sys.stdout, indentCols=4, printed=set()):
		CodePrinter.__init__(self, outf, indentCols)
		self.printed = printed

		self.pcurrent = {}
		self.tcurrent = {}

	def dgen(self, ast, pparse=0, tparse=0, protocols="", types="", verbosity=1):
		self.protocols = protocols
		self.types = types
		self.pparse = pparse
		self.tparse = tparse

		self.verbosity = verbosity
		ast.accept(self)

	def visitTranslationUnit(self, tu):
		self.printed.add(tu.filename)
		Visitor.visitTranslationUnit(self, tu)

	def visitCxxInclude(self, inc):
		
		pass

	def visitProtocolInclude(self, inc):
		
		pass

	def visitProtocol(self, p):
		# if we don't have an output list or protocol parsing
		# flag is not set, don't parse the protocol
		if not isinstance(self.protocols, list) or not self.pparse:
			return

		self.pcurrent["name"] = p.name
		self.pcurrent["parent"] = {}
		self.pcurrent["child"] = {}
		self.pcurrent["both"] = {}
		
		Visitor.visitProtocol(self, p)

		self.protocols.append(self.pcurrent)

		# delete the current protocol list, in case we have more than one
		# delcaration in one file
		self.protocols = {}

	def visitStructDecl(self, decl):
		# if we don't have an output list or types parsing
		# flag is not set, don't parse the types
		if not isinstance(self.types, list) or not self.tparse:
			return

		self.tcurrent["name"] = decl.name
		self.tcurrent["namespace"] = "::".join(ns.name for ns in decl.namespaces)
		self.tcurrent["type"] = "struct"
		self.tcurrent["fields"] = {}

		for field in decl.fields:
			self.tcurrent["fields"][str(field.typespec)] = field.name 

		self.types.append(self.tcurrent)
		self.tcurrent = {}

	def visitUnionDecl(self, decl):
		# if we don't have an output list or types parsing
		# flag is not set, don't parse the types
		if not isinstance(self.types, list) or not self.tparse:
			return

		self.tcurrent["name"] = decl.name
		self.tcurrent["namespace"] = "::".join(ns.name for ns in decl.namespaces)
		self.tcurrent["type"] = "union"
		self.tcurrent["fields"] = []		

		for comp in decl.components:
			self.tcurrent["fields"].append(str(comp.spec))

		self.types.append(self.tcurrent)
		
		# delete the current type list, in case we have more than one
		# delcaration in one file
		self.tcurrent = {}	

	def visitManagerStmt(self, mgr):
		
		pass

	def visitManagesStmt(self, mgs):
		
		pass

	def visitUsingStmt(self, us):
		
		pass

	def visitTypeSpec(self, ts):

		pass

	def visitMessageDecl(self, msg):
		# TODO: also include 'both' if that exists
		if msg.direction.pretty == "out":
			subname = "child" 
		elif msg.direction.pretty == "in":
			subname = "parent"
		elif msg.direction.pretty == "inout":
			subname = "both"
		else:
			print >>sys.stderr, "Unknown direction: " + msg.direction.pretty
			exit(1)

		# TODO: check if message already exists? (which shouldn't be the case)
		self.pcurrent[subname][msg.name] = []

		# if there are no arguments, there is nothing else to do
		if len(msg.inParams) == 0 and len(msg.outParams) == 0:
			return

		# depending on the verbosity, generate the output
		if self.verbosity == 1 or self.verbosity == 2:
			inJoin = ", ".join(str(p.typespec) + " " + p.name for p in msg.inParams)
			outJoin = ", ".join(str(p.typespec) + " " + p.name for p in msg.outParams)

			if self.verbosity == 1:
				self.pcurrent[subname][msg.name] = "("+outJoin+") ("+inJoin+")"
			if self.verbosity == 2:
				self.pcurrent[subname][msg.name] = {}
				if len(inJoin): self.pcurrent[subname][msg.name]["in"] = inJoin
				if len(outJoin): self.pcurrent[subname][msg.name]["out"] = outJoin

		elif self.verbosity == 3 or self.verbosity == -1:
			self.pcurrent[subname][msg.name] = {}
			if len(msg.inParams): self.pcurrent[subname][msg.name]["in"] = {}
			if len(msg.outParams): self.pcurrent[subname][msg.name]["out"] = {}

			for inp in msg.inParams:
				# if it is a builtin type then don't try to resolve it
				if self.verbosity == 3 or str(inp.typespec) in ipdl.builtin.Types:
					data = inp.name
				else:
					data = filter(lambda dtype: dtype['name'] == str(inp.typespec), self.types['types'])

				if isinstance(data, list) and len(data) > 0:
					data = data[0]['fields']

				self.pcurrent[subname][msg.name]["in"][str(inp.typespec)] = data
			for outp in msg.outParams:
				# if it is a builtin type then don't try to resolve it
				if self.verbosity == 3 or str(outp.typespec) in ipdl.builtin.Types:
					data = outp.name
				else:
					data = filter(lambda dtype: dtype['name'] == str(outp.typespec), self.types['types'])

				if isinstance(data, list) and len(data) > 0:
					data = data[0]['fields']

				self.pcurrent[subname][msg.name]["out"][str(outp.typespec)] = data