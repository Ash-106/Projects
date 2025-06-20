from crewmate import CrewMate
from heap import Compare
from heap import Heap
from treasure import Treasure
import random

class StrawHatTreasury:
    
    def __init__(self, m):
        
        self.li = []
        for i in range(0,m):
            self.li.append(CrewMate())
            
        self.a = Heap(Compare.comp1,self.li)
        self.l = []
        
        self.Tres = []
        self.Tres2 = []
        self.ct = 0


    def add_treasure(self, treasure):
       
       b = self.a.extract()
       cths = b.H.heapsize
       if b.H.heapsize!=0:
            b.proCess(treasure.arrival_time)
       self.ct -= (cths - b.H.heapsize)
       b.curr_time = treasure.arrival_time
       b.H.insert(treasure)
       b.Ld += treasure.size
       self.l.append(b)
       self.a.insert(b)
       self.Tres.append(treasure)
       self.ct+=1
       
    
    def get_completion_time(self):

        c = 0
        st = 0
        
        siz = len(self.l)
        d = 0

        if siz == self.ct and len(self.l) <= self.a.heapsize:
            while d < siz and c < self.ct:

                curCr = self.l[d]
                ct = curCr.curr_time
                lio = []
                while curCr.H.heapsize!=0:
                    curOb = curCr.H.extract()
                    c+=1
                    lio.append(curOb)
                for i in lio:    
                    i.completion_time = curCr.curr_time + i.size 
                    curCr.curr_time = i.completion_time
                for i in lio:
                    curCr.H.insert(i)
                
                curCr.curr_time = ct
                d+=1
            

            self.Tres.sort(key = lambda t : t.id)
            return self.Tres




        else:

            crewl = []

            while self.a.heapsize!=0 and c < self.ct:

                curCr = self.a.extract()
                ct = curCr.curr_time
                lio = []
                while curCr.H.heapsize!=0:
                    curOb = curCr.H.extract()
                    c+=1
                    lio.append(curOb)
                for i in lio:    
                    i.completion_time = curCr.curr_time + i.size 
                    curCr.curr_time = i.completion_time
                for i in lio:
                    curCr.H.insert(i)
                
                curCr.curr_time = ct
                crewl.append(curCr)
            
            for i in crewl:
                self.a.insert(i)

            self.Tres.sort(key = lambda t : t.id)
            return self.Tres
