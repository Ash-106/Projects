from bin import Bin
from avl import AVLTree
from object import Object, Color
from exceptions import NoBinFoundException
from node import Node
from node import NodeOb

class GCMS:
    def __init__(self):
        # Maintain all the Bins and Objects in GCMS
        self.T = AVLTree(1)
        self.O = AVLTree(0)
        self.B = AVLTree(0)
        
    def add_bin(self, bin_id, capacity):
        
        temp  = Node(capacity,bin_id)
        temp2 = Node(capacity,bin_id)
        #print(temp.si)
        self.T.root = self.T.insert(self.T.root,temp)
        self.B.root = self.B.insert(self.B.root,temp2)
        
    def add_object(self, object_id, size, colori):
        #print(size)
        #print(self.T.root.si)

        n = self.T.findadd_bin(self.T.root,object_id,size,colori)
        
        if n == None:
            raise NoBinFoundException
        b = Node(n.si,n.ID)
        b.M = n.M
        b.left = n.left
        b.right = n.right
        b.height = n.height
            
        if n.si < size:
            raise NoBinFoundException
                
        self.T.root = self.T.delete(self.T.root,n)
        
        b.add_object(object_id,size)

        self.T.root = self.T.insert(self.T.root,b)
        #print(root.si)
        
        bi = self.B.search(self.B.root,b.ID,0)
       #bi.l.append(object_id)
       
        bi.si -= size
        O1 = NodeOb(object_id)
        bi.M.root = bi.M.insert(bi.M.root,O1)
        #o = Object(object_id,size,colori,b.ID)

        ob = self.O.search(self.O.root,object_id,0)
        if ob is None:
            o = Object(object_id,size,colori,b.ID)
            self.O.root = self.O.insert(self.O.root,o)

        else:
            ob.BIN_id = bi.ID
        
        
        

    def delete_object(self, object_id):
        # Implement logic to remove an object from its bin
        #print("Del")
        
        se = self.O.search(self.O.root,object_id,0)
        seID = se.BIN_id
        BTree = self.B.search(self.B.root,seID,0)
        CTree = Node(BTree.si,BTree.ID)
        CTree.left = BTree.left
        CTree.right = BTree.right
        CTree.height = BTree.height
        CTree.M = BTree.M
        #CTree.l = BTree.l
        self.T.root = self.T.delete(self.T.root,BTree)

        siz = BTree.si
        #print(siz)
        #print(BTree.ID)
        BTree.M.root = BTree.M.delete(BTree.M.root,se)
        #BTree.l.remove(object_id)
        
        BTree.si += se.size
        CTree.si += se.size
        self.T.root = self.T.insert(self.T.root,CTree)
        
        self.O.root = self.O.deleteOb(self.O.root,se)
        
    def bin_info(self, bin_id):
        # returns a tuple with current capacity of the bin and the list of objects in the bin (int, list[int])
        bi = self.B.search(self.B.root,bin_id,0)
        l = []
        self.addlist(bi.M.root,l)
        return ((bi.si,l))
        

    def object_info(self, object_id):
        # returns the bin_id in which the object is stored
        t = self.O.search(self.O.root,object_id,0)
        return(t.BIN_id)
        
    def printBtree(self):
        print(self.B.root.ID)
        print(self.B.root.left.ID)
        print(self.B.root.right.ID)
        
    def in_order_print(self, root, level=0):
        if root is not None:
            self.in_order_print(root.left, level + 1)  # Traverse the left subtree
            print(' ' * 4 * level + '->', f"Node: {root.ID}, Size: {root.si}")  # Print the current node
            self.in_order_print(root.right, level + 1)  # Traverse the right subtree

    def in_order_printO(self, root, level=0):
        if root is not None:
            self.in_order_printO(root.left, level + 1)  # Traverse the left subtree
            print(' ' * 4 * level + '->', f"Node: {root.ID}")  # Print the current node
            self.in_order_printO(root.right, level + 1)  # Traverse the right subtree
   
    def addlist(self,node,l):
        if node!=None:
            l.append(node.ID)
            if node.left:
                self.addlist(node.left,l)
            if node.right:
                self.addlist(node.right,l)