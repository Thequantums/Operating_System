#!/usr/bin/python

import csv, sys
from collections import defaultdict
block = defaultdict(list)

class Superblock:
    def __init__(self,args):
        self.num_blocks = int(args[1])
        self.num_inodes = int(args[2])
        self.block_size = int(args[3])
        self.inode_size = int(args[4])
        self.blocks_per_group = int(args[5])
        self.inodes_per_group = int(args[6])
        self.first_inode = int(args[7])

class Group:
    def __init__(self, args):
        self.group_num = int(args[1])
        self.blocks_in_group = int(args[2])
        self.inodes_in_group = int(args[3])
        self.free_blocks = int(args[4])
        self.free_inodes = int(args[5])
        self.block_bitmap = int(args[6])
        self.inode_bitmap = int(args[7])
        self.inode_table = int(args[8])

class Bfree:
    def __init__(self,args):
        self.free_location = int(args[1])

class Ifree:
    def __init__(self, args):
        self.free_location = int(args[1])

class Inode:
    def __init__(self, args):
        self.inode_num = int(args[1])
        self.file_type = args[2]
        #self.mode = oct(args[3])
        self.owner = int(args[4])
        self.group = int(args[5])
        self.link_count = int(args[6])
        self.file_size = int(args[10])
        self.blocks = int(args[11])
        temp_block_addr = args[12:]
        self.block_addr = []
        for item in temp_block_addr:
            self.block_addr.append(int(item))
        
class Directory:
    def __init__(self,args):
        self.parent_inode = int(args[1])
        self.offset = int(args[2])
        self.inode = int(args[3])
        self.entry_len = int(args[4])
        self.name_len = int(args[5])
        self.name = str(args[6])

class Indirect:
    def __init__(self,args):
        self.owning_inode = int(args[1])
        self.level = int(args[2])
        self.offset = int(args[3])
        self.scanned_block = int(args[4])
        self.ref_block = int(args[5])

def isinlist(num,list):
	for i in range(len(list)):
		if list[i] == num:
			return 1
	return 0

def get_offset(j,num_ide):
	if j <= 12:
		offset = j
	if j == 13:
		offset = 12 + (num_ide) 
	if j == 14:
		offset = 12 + num_ide + num_ide * num_ide
	return offset

def get_indirection_type(level):
	if level == 1 or level == 12:
		type = 'INDIRECT'
	if level == 2 or level == 13:
		type = 'DOUBLE INDIRECT'
	if level == 3 or level == 14:
		type = 'TRIPLE INDIRECT'
	return type

def is_i_infreelist(inode,ifree):
	for i in range(len(ifree)):
		if inode == ifree[i].free_location:
			return 1
	return 0

def is_i_allocated(num,inode):
	for i in range(len(inode)):
		if num == inode[i].inode_num:
			return 1
	return 0

def block_inode_audits(super_block,group,bfree,ifree,inode,indir):
    #store all reference blocks in block list, can be in bfree, indirect, and inode lines
    #check invalid and reserved block, only check in inode lines and indir
    num_ide = super_block.block_size/4
    lower_bound = group[0].inode_table + (group[0].inodes_in_group * super_block.inode_size)/super_block.block_size
    #usually num_ide = 256
    #check invalid and then reservered blocks in inode lines

    for i in range(len(inode)):
        for j in range(len(inode[i].block_addr)):
	    if inode[i].block_addr[j] < 0 or inode[i].block_addr[j] >= group[0].blocks_in_group:
		offset = get_offset(j,num_ide)
                if j <=11:
		    sys.stdout.write('INVALID BLOCK ' + str(inode[i].block_addr[j]) + ' ' + 'IN INODE ' + str(inode[i].inode_num) + ' AT OFFSET ' + str(offset) + '\n')
		else:
		    type = get_indirection_type(j)
                    sys.stdout.write('INVALID ' + type + ' BLOCK ' + str(inode[i].block_addr[j]) + ' ' + 'IN INODE ' + str(inode[i].inode_num) + ' AT OFFSET ' + str(offset) + '\n')
                	    
            if inode[i].block_addr[j] > 0 and inode[i].block_addr[j] < lower_bound:
		#step in reserved block
		offset = get_offset(j,num_ide)
                if j <=11:
		    sys.stdout.write('RESERVED BLOCK ' + str(inode[i].block_addr[j]) + ' ' + 'IN INODE ' + str(inode[i].inode_num) + ' AT OFFSET ' + str(offset) + '\n')
		else:
		    type = get_indirection_type(j)
                    sys.stdout.write('RESERVED ' + type + ' BLOCK ' + str(inode[i].block_addr[j]) + ' ' + 'IN INODE ' + str(inode[i].inode_num) + ' AT OFFSET ' + str(offset) + '\n')


    #check invalid and reserved blocks in indirect

    for i in range(len(indir)):
	if indir[i].ref_block < 0 or indir[i].ref_block >= group[0].blocks_in_group:
	    type = get_indirection_type(indir[i].level)
	    sys.stdout.write('INVALID ' + type + ' BLOCK ' + str(indir[i].ref_block) + ' IN INODE ' + str(indir[i].owning_inode) + ' AT OFFSET ' + str(indir[i].offset) + '\n')
	if indir[i].ref_block > 0 and indir[i].ref_block < lower_bound:
	    type = get_indirection_type(indir[i].level)
	    sys.stdout.write('RESERVERD ' + type + ' BLOCK ' + str(indir[i].ref_block) + ' IN INODE ' + str(indir[i].owning_inode) + ' AT OFFSET ' + str(indir[i].offset) + '\n')
	
    # check unreference blocks
    #scan through bfree to get all reference block by appending them to list
    list = []
    for i in range(len(bfree)):
	list.append(bfree[i].free_location)
    for i in range(len(inode)):
	for j in range(len(inode[i].block_addr)):
	    list.append(inode[i].block_addr[j])
    for i in range(len(indir)):
	list.append(indir[i].ref_block)
	
    #find unreference blocks starting from reserverd block (lowerbound value)
    index = lower_bound
    upper_bound = group[0].blocks_in_group # if upper_boubd = 64, block pointer must be less than 64. cant be 64
    while index != upper_bound:
        if isinlist(index,list) == 0:
	    #found unreference block
	    sys.stdout.write('UNREFERENCED BLOCK ' + str(index) + '\n')
	index = index + 1 
	
    # find allocated blocks on free list
    for i in range(len(bfree)):
	for j in range(len(inode)):
	    for k in range(len(inode[j].block_addr)):
		if(bfree[i].free_location == inode[j].block_addr[k]):
	            #found error
		    sys.stdout.write('ALLOCATED BLOCK ' + str(bfree[i].free_location) + ' ON FREELIST\n')
        for l in range(len(indir)):
	    if indir[l].ref_block == bfree[i].free_location:
	        #found error
	        sys.stdout.write('ALLOCATED BLOCK ' + str(bfree[i].free_location) + ' ON FREELIST\n')
    	
    # find multi-references
    list_dup = []
    dup = []
    #first find the duplicated block number
    for i in range(len(list)):
        if isinlist(list[i],list_dup) == 1 and list[i] >= lower_bound and list[i] < upper_bound and is_i_infreelist(list[i],bfree) == 0:
            dup.append(list[i])	
	list_dup.append(list[i])
    
    #print all dup locations
    for i in range(len(dup)):
        for j in range(len(inode)):
            for k in range(len(inode[j].block_addr)):
                if dup[i] == inode[j].block_addr[k]:
                    offset = get_offset(k,num_ide)
                    if k <= 11:
                        sys.stdout.write('DUPLICATE BLOCK ' + str(dup[i]) + ' IN INODE ' + str(inode[j].inode_num) + ' AT OFFSET ' + str(offset) + '\n')
                    else:
			type = get_indirection_type(k)
                        sys.stdout.write('DUPLICATE ' + type + ' BLOCK ' + str(dup[i]) + ' IN INODE ' + str(inode[j].inode_num) + ' AT OFFSET ' + str(offset) + '\n')       
	for l in range(len(indir)):
            if indir[l].ref_block == dup[i]:
                type = get_indirection_type(indir[l].level)
                sys.stdout.write('DUPLICATE ' + type + ' BLOCK ' + str(indir[i].ref_block) + ' IN INODE ' + str(indir[i].owning_inode) + ' AT OFFSET ' + str(indir[i].offset) + '\n')				
	#
	# Now we audit the inode
	#			
	
    #find allocated inode on freelist
    for i in range(len(ifree)):
        for j in range(len(inode)):
            if ifree[i].free_location == inode[j].inode_num:
                sys.stdout.write('ALLOCATED INODE ' + str(ifree[i].free_location) + ' ON FREELIST\n')
	
    #find unreferenced inode
    # 1 - 10 inode numbers are reservered, inode 2 can be present in csv coz it is a root directory
    i = super_block.first_inode
    while i != (group[0].inodes_in_group + 1):
        if is_i_infreelist(i,ifree) == 0 and is_i_allocated(i,inode) == 0:
            sys.stdout.write('UNALLOCATED INODE ' + str(i) + ' NOT ON FREELIST\n')	
        i = i + 1
	
	
	
def main():
    filename = sys.argv[1]
    group = []
    bfree = []
    ifree = []
    inode = []
    dirent = []
    indir = []
    
    with open(filename) as csv_file:
        csv_reader = csv.reader(csv_file, delimiter=',')
        for row in csv_reader:
            if row[0] == "SUPERBLOCK":
                super_block = Superblock(row)
            elif row[0] == "GROUP":
                group.append(Group(row))
            elif row[0] == "BFREE":
                bfree.append(Bfree(row))
            elif row[0] == "IFREE":
                ifree.append(Ifree(row))
            elif row[0] == "INODE":
                inode.append(Inode(row))
            elif row[0] == "DIRENT":
                dirent.append(Directory(row))
            elif row[0] == "INDIRECT":
                indir.append(Indirect(row))
            else:
                sys.exit('Unknown csv entry!')
    block_inode_audits(super_block,group,bfree,ifree,inode,indir)

#    print(inode[0].block_addr) #testing
                
    
#    log = open(filename, 'r')
#    for line in log:
#        print(line)

if __name__== "__main__":
    main()
