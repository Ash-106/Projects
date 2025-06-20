from node import Node
from bin import Bin
from object import Color
from object import Object

def comp_1(node_1, node_2):
    if node_1.si==node_2.si:
        return node_1.ID < node_2.ID
        
    return node_1.si < node_2.si

def comp_2(node_1,node_2):
    return node_1.ID < node_2.ID

class AVLTree:
    def __init__(self, a):
        if a==1:
            compare_function = comp_1
        else:
            compare_function = comp_2
        self.root = None
        self.size = 0
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
            root.si = temp.si
            root.ID = temp.ID
            root.l = temp.l
            temp.right = root.right
            temp.left = root.left
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
    

    def deleteOb(self, root, object):

        if not root:
            return root
        
        if self.comparator(object,root):
            
            root.left = self.deleteOb(root.left, object)

        elif self.comparator(root,object):

            root.right = self.deleteOb(root.right, object)

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
            
            root.right = self.deleteOb(root.right, temp)
            
            if temp!=None:
                root.size = temp.size
                root.ID = temp.ID
                root.color = temp.color
                root.BIN_id = temp.BIN_id
                temp.right = root.right
                temp.left = root.left
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
        """y = z.right
        T2 = y.left
        y.left = z
        z.right = T2
        z.height = 1 + max(self.height(z.left), self.height(z.right))
        y.height = 1 + max(self.height(y.left), self.height(y.right))
        return y"""
            
        if z.right is None:
            return z  

        y = z.right
        T2 = y.left

    # Perform the rotation
        y.left = z
        z.right = T2

    # Update heights
        z.height = 1 + max(self.height(z.left), self.height(z.right))
        y.height = 1 + max(self.height(y.left), self.height(y.right))

        return y

    def rightrotation(self, z):
        """print("Rotating")
        print(z.ID)
        y = z.left
        T3 = y.right
        y.right = z
        z.left = T3
        z.height = 1 + max(self.height(z.left), self.height(z.right))
        y.height = 1 + max(self.height(y.left), self.height(y.right))
        return y"""

      
        
    
    # Check if the left child exists before proceeding
        if z.left is None:
            
            return z  # Return the node as it is since rotation can't happen

        y = z.left
        T3 = y.right
    
    # Perform the rotation
        y.right = z
        z.left = T3
    
    # Update heights
        z.height = 1 + max(self.height(z.left), self.height(z.right))
        y.height = 1 + max(self.height(y.left), self.height(y.right))
    
        return y

    def min_value_node(self, root):
        current = root
        while current.left:
            current = current.left
        return current

    def search(self, root, value,a):
        if a==1:
            if not root or root.si == value:
                return root
            if root.si < value:
                return self.search(root.right, value,a)
            return self.search(root.left, value,a)
            
        else:
            if not root or root.ID == value:
                return root
            if root.ID < value:
                return self.search(root.right, value,0)
            return self.search(root.left, value,0)

    def findadd_bin(self,root,object_id,size,colori):
        
        if colori == Color.BLUE:
            return self.Bl(root,object_id,size)
               
        elif colori== Color.YELLOW:
            return self.Y(root,object_id,size)
                
        elif colori== Color.RED:
            return self.R(root,object_id,size)
            
        elif colori==Color.GREEN:
            return self.G(root,object_id,size)


    def Bl(self,root,object_id,size):
        n = root
        if n.si < size:
            if n.right== None:
                return None
            n = self.Bl(n.right,object_id,size)
            return n
        else:
            if n.left==None:
                return n
            n = self.Bl(n.left,object_id,size)
            if n==None:
                    return root
            else:
                return n  
            
    def Y(self,root,object_id,size):
                n = root
                if n.si < size:
                    
                    if n.right:
                        return self.Y(n.right,object_id,size)
                    else:
                        return n
                
                else:
                    if n.left:
                        b = self.Y(n.left,object_id,size)
                        
                        if  b.si < n.si and b.si >= size:
                            
                            return b
                        else:
                            
                            if n.right:
                                c = self.Y(n.right,object_id,size)
                               
                                if c!=None and c.si == n.si:
                                    return c
                                else:
                                    return n
                            else:
                                return n

                    else:
                        if n.right:
                            c = self.Y(n.right,object_id,size)
                            if c.si == n.si:
                                return c
                            else:
                                return n
                        else:
                            return n

    def R(self,root,object_id,size):
            n = root

            if n.right!=None:
                if n.right.si > n.si:
                    n = self.R(n.right,object_id,size)
                    return n
                else:
                    if n.left:
                        b = self.R(n.left,object_id,size)
                        if b.si == n.si:
                            return b
                        else:
                            n = self.R(n.right,object_id,size)
                            if n.si == root.si:
                                return root
                            else:
                                return n
                    
                    else:
                        n = self.R(n.right,object_id,size)
                        if n.si == root.si:
                            return root
                        else:
                            return n

            else:
                if n.left:
                    c =  self.R(n.left,object_id,size)   
                    if c.si==n.si:
                        return c
                    else:
                        return n
                else:
                    return n

    def G(self,root,object_id,size):
            n = self.root

            while n.right!=None:
                n = n.right
                
            return n

    def printo(self,root):
        if root is None:
            return
        else:
            print(root.si)
            self.printo(root.left)
            self.printo(root.right)
        
        
        
        
                 
                
                
                
                
            
    
    