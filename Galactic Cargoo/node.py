from bin import Bin


class Node:
    def __init__(self,size,ID):
        self.si = size
        self.ID = ID
        self.l=[]
        self.left = None
        self.right = None
        self.height = 1
        self.M = AVL()
        
    def add_object(self,object_id,size):
            self.si-=size
            self.l.append(object_id)     

class NodeOb:
     def __init__(self,ID):
        self.ID = ID
        self.left = None
        self.right = None
        self.height = 1

def comp_2(node_1,node_2):
    return node_1.ID < node_2.ID
   
class AVL:
    def __init__(self):
        
        compare_function = comp_2
        self.root = None
        self.ID = 0
        self.comparator = compare_function
        
    def height(self,node):
        if not node:
            return 0
        else:
            return node.height
            
    def balance(self,node):
        if not node:
            return 0
        else:
            return self.height(node.left) - self.height(node.right)
        
    def insert(self,root,node):
        if root is None:
            node.left = None
            node.right = None
            node.height = 1
            return node 
            
        else:
            if self.comparator(node,root) :
                root.left = self.insert(root.left,node)
            elif self.comparator(root,node):
                root.right = self.insert(root.right,node)
        
        root.height = 1 + max(self.height(root.left), self.height(root.right))
        balance = self.balance(root)
        #print(f"Balance factor at node {root.si}: {balance}")
        
        if balance > 1 and self.comparator(node,root.left):
            #print(f"Right rotation at node {root.si}")
            root =  self.rightrotation(root)
            
        if balance < -1 and self.comparator(root.right,node):
            #print(f"Left rotation at node {root.si}")
            root = self.leftrotation(root)
        
        if balance > 1 and self.comparator(root.left,node):
                root.left = self.leftrotation(root.left)
                root= self.rightrotation(root)
            
        if balance < -1 and self.comparator(node,root.right):
            root.right = self.rightrotation(root.right)
            root =  self.leftrotation(root)
        
        return root

    def delete(self, root, node):
        
        if not root:
            return root

        if self.comparator(node,root):
            
            root.left = self.delete(root.left, node)
        elif self.comparator(root,node):
            
            root.right = self.delete(root.right, node)
        else:
            if not root.left:
                temp = root.right
                root = None
                return temp
            elif not root.right:
                temp = root.left
                root = None
                return temp

            temp = self.min_value_node(root.right)
        
            
            root.right = self.delete(root.right, temp)
            root.ID = temp.ID
            temp.eft = root.left
            temp.right = root.right
            temp.height = root.height
            

        if root==None:
            
            return root
        
        
        root.height = 1 + max(self.height(root.left), self.height(root.right))
        #print(root.height)
        balance = self.balance(root)
        #print(balance)

        # Left rotation
        if balance > 1 and self.balance(root.left) >= 0:
            root =  self.rightrotation(root)

        # Right rotation
        if balance < -1 and self.balance(root.right) <= 0:
            root =  self.leftrotation(root)

        # Left-Right rotation
        if balance > 1 and self.balance(root.left) < 0:
            root.left = self.leftrotation(root.left)
            root =  self.rightrotation(root)

        # Right-Left rotation
        if balance < -1 and self.balance(root.right) > 0:
            root.right = self.rightrotation(root.right)
            root =  self.leftrotation(root)

        return root
    

    def leftrotation(self, z):
        y = z.right
        T2 = y.left

        y.left = z
        z.right = T2

        z.height = 1 + max(self.height(z.left), self.height(z.right))
        y.height = 1 + max(self.height(y.left), self.height(y.right))

        return y

    def rightrotation(self, z):
        y = z.left
        T3 = y.right

        y.right = z
        z.left = T3

        z.height = 1 + max(self.height(z.left), self.height(z.right))
        y.height = 1 + max(self.height(y.left), self.height(y.right))

        return y

    def min_value_node(self, root):
        current = root
        while current.left:
            current = current.left
        return current

    def search(self, root, value,a):
        
            if not root or root.ID == value:
                return root
            if root.ID < value:
                return self.search(root.right, value,0)
            return self.search(root.left, value,0)
