class Compare:

    def comp1(self,O1,O2):
       return O1.Ld + O1.curr_time < O2.Ld + O2.curr_time
    
    def comp2(self,t1,t2):
        if t1.arrival_time + t1.size!= t2.arrival_time + t2.size:
            return t1.arrival_time + t1.size <  t2.arrival_time + t2.size

        else:
            return t1.id < t2.id

    def comp3(self,T1,T2):
        return T1.completion_time < T2.completion_time
    

class Heap:
    
    def __init__(self, comparison_function, init_array):
        
        self.he = init_array
        self.comp = comparison_function
        self.heapsize = len(init_array)
        if len(init_array):
            self.HeapifyD(0)
        
    def insert(self,O1):

        self.he.append(O1)
        self.heapsize+=1
        self.HeapifyU(self.heapsize-1)
        
    def leftCh(self,i):
        return 2*i + 1

    def Rch(self,i):
        return 2*i + 2 

    def Parent(self,i):
        return (i-1)//2
    
    def extract(self):
    
        if self.heapsize==0:
            return None
        elif self.heapsize==1:
            a = self.top()
            self.heapsize-=1
            self.he.pop()
            return a
        
        ret = self.top()
        self.he[0] = self.he.pop()
        self.heapsize -=1
        if self.heapsize > 0:
            self.HeapifyD(0)
        return ret
        # Write your code here
        
    def top(self):
        if self.heapsize==0:
            return None
        return self.he[0]
    
    def HeapifyD(self,i):
        if self.comp == Compare.comp1 or self.comp==Compare.comp2 or self.comp ==Compare.comp3:
        
            l = self.leftCh(i)
            r = self.Rch(i)
            curr = i

        
            if l < self.heapsize and self.comp(self,self.he[l], self.he[i]):
                curr = l
                    
            if r < self.heapsize and self.comp(self,self.he[r], self.he[curr]):
                curr = r

            if curr != i:
                self.he[i], self.he[curr] = self.he[curr], self.he[i]  
                self.HeapifyD(curr) 
         
        else:
            l = self.leftCh(i)
            r = self.Rch(i)
            curr = i

            if l < self.heapsize and self.comp(self.he[l], self.he[i]):
                curr = l
                    
            if r < self.heapsize and self.comp(self.he[r], self.he[curr]):
                curr = r

            if curr != i:
                self.he[i], self.he[curr] = self.he[curr], self.he[i]  
                self.HeapifyD(curr) 


    def HeapifyU(self,i):
        if self.comp == Compare.comp1 or self.comp==Compare.comp2 or self.comp ==Compare.comp3:
            if i > 0:
                par = self.Parent(i)
                # Compare using the comp function
                if self.comp(self,self.he[i], self.he[par]):
                    self.he[i], self.he[par] = self.he[par], self.he[i]  
                    self.HeapifyU(par)

        else:
            if i > 0:
                par = self.Parent(i)
                # Compare using the comp function
                if self.comp(self.he[i], self.he[par]):
                    self.he[i], self.he[par] = self.he[par], self.he[i]  
                    self.HeapifyU(par)



        






