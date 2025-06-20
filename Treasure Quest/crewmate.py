from heap import Heap
from heap import Compare
from treasure import Treasure


class CrewMate:
   
    def __init__(self):

        self.Ld =0
        self.H = Heap(Compare.comp2,[])
        self.curr_time = 0
        
    def proCess(self,ti):

        i = self.curr_time
        while i < ti:
            if self.H.heapsize > 0:
                temp = self.H.extract()
                if temp.size > (ti-i):
                    temp.size -= (ti-i)
                    self.Ld -= (ti-i)
                    self.H.insert(temp)
                    i = ti
                else:
                    temp.completion_time = i+temp.size 
                    self.Ld -= (temp.size)
                    i += temp.size 
                    temp.size = 0
                    
            else:
                break



        

    
    
    