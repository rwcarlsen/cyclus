#! /usr/bin/env python

from nose.tools import assert_false, assert_true, assert_equal
from numpy.testing import assert_array_equal
import os
import tables
import numpy as np
from tools import check_cmd
from helper import tables_exist, find_ids, exit_times, \
    h5out, sqliteout, clean_outs, to_ary, which_outfile

"""Tests"""
def test_inventories_false():
    """Testing for inventory and compact inventory table non-creation.
    """
    clean_outs()
    # Cyclus simulation input with inventory table
    sim_inputs = ["./input/inventory_false.xml", "./input/inventory_compact_false.xml"]
    paths = [["/ExplicitInventory"], ["/ExplicitInventoryCompact"]]
    for sim, path in zip(sim_inputs, paths):
        holdsrtn = [1]  # needed because nose does not send() to test generator
        outfile = which_outfile()
        cmd = ["cyclus", "-o", outfile, "--input-file", sim]
        yield check_cmd, cmd, '.', holdsrtn
        rtn = holdsrtn[0]
        if rtn != 0:
            return  # don't execute further commands

        # Ensure tables do not exist
        assert_false, tables_exist(outfile, path)
        if tables_exist(outfile, path):
	    print('Inventory table exists despite false entry in control section of input file.')
            outfile.close()
            clean_outs()
            return  # don't execute further commands

def test_inventories():
    """Testing for inventory and compact inventory table creation.
    """
    clean_outs()
    # Cyclus simulation input with inventory table
    sim_inputs = ["./input/inventory.xml", "./input/inventory_compact.xml"]
    paths = [["/ExplicitInventory"], ["/ExplicitInventoryCompact"]]
    for sim, path in zip(sim_inputs, paths):
        holdsrtn = [1]  # needed because nose does not send() to test generator
        outfile = which_outfile()
        cmd = ["cyclus", "-o", outfile, "--input-file", sim]
        yield check_cmd, cmd, '.', holdsrtn
        rtn = holdsrtn[0]
        if rtn != 0:
            return  # don't execute further commands
            
        # Check if inventory tables exist
        assert_true, tables_exist(outfile, path)
        if not tables_exist(outfile, path):
	    print('Inventory table does not exist despite true entry in control section of input file.')
            outfile.close()
            clean_outs()
            return  # don't execute further commands
            
        # Get specific table
	table = path[0]
        if outfile == h5out:
            output = tables.open_file(h5out, mode = "r")
      	    inventory = output.get_node(table)[:]
      	    compositions = output.get_node("/Compositions")[:]
            output.close()
        else:
            conn = sqlite3.connect(outfile)
            conn.row_factory = sqlite3.Row
            cur = conn.cursor()
            exc = cur.execute
	    sqltable = table.replace('/', '')
	    sql = "SELECT * FROM %s" % sqltable
            inventory = exc(sql).fetchall()
            compositions = exc('SELECT * FROM Compositions').fetchall()
            conn.close()
        
	# Test that quantity increases as expected with k=1
	if "Compact" not in table:
	    time = to_ary(inventory, "Time")
	    quantity = to_ary(inventory, "Quantity")
	    nucid = to_ary(inventory, "NucId")
	    massfrac = to_ary(compositions, "MassFrac")
	    nucid_comp = to_ary(compositions, "NucId") 
	    expected_quantity = []
	    expected_nucid = []
	    repeat = 100
	    for t in time:
	    	if t != repeat:
	    	    expected_quantity.append((t+1)*massfrac[1])
		    expected_nucid.append(nucid_comp[1])
		else:
	    	    expected_quantity.append((t+1)*massfrac[0])
		    expected_nucid.append(nucid_comp[0])
		t = repeat
	    assert_array_equal, quantity, expected_quantity
	    assert_array_equal, nucid, expected_nucid
            return  # don't execute further commands
	else:
	    # composition will be the same in compact inventory
	    return  # don't execute further commands

        clean_outs()

