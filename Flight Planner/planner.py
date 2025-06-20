from flight import Flight

class Stack:
    def __init__(self):
        #YOU CAN (AND SHOULD!) MODIFY THIS FUNCTION
        self.l = []
        
    def push(self,x):
        self.l.append(x)
        
    def pop(self):
        if len(self.l)!=0:
            a = self.l[-1]
            self.l.pop()
            return a
        else:
            return -1
        
    def isEmpty(self):
        return len(self.l)==0
        
    def top(self):
        if len(self.l)!=0:
            return self.l[-1]
        
class Queue:
    def __init__(self):
        self.stack_in = Stack()  
        self.stack_out = Stack() 

    def append(self, value):
        self.stack_in.push(value)

    def pop(self):
        if self.stack_out.isEmpty():  # Only transfer if stack_out is empty
            while not self.stack_in.isEmpty():
                self.stack_out.push(self.stack_in.pop())
        if not self.stack_out:  # If stack_out is still empty, queue is empty
            raise IndexError("dequeue from an empty queue")
        return self.stack_out.pop()
        

    # Check if the queue is empty  
    def is_empty(self):
        return self.stack_in.isEmpty() and self.stack_out.isEmpty()

    # Get the size of the queue
    def size(self):
        return len(self.stack_in.l) + len(self.stack_out.l)

class Compare:

    def comp1(self,O1,O2):
       return O1[0] < O2[0]
    
    def comp2(self,O1,O2):
        if O1[0]!=O2[0]:
            return O1[0] < O2[0]
        else:
            return O1[1] < O2[1]

class Heap:
    
    def __init__(self, init_array):
        from flight import Flight

class Stack:
    def __init__(self):
        #YOU CAN (AND SHOULD!) MODIFY THIS FUNCTION
        self.l = []
        
    def push(self,x):
        self.l.append(x)
        
    def pop(self):
        if len(self.l)!=0:
            a = self.l[-1]
            self.l.pop()
            return a
        else:
            return -1
        
    def isEmpty(self):
        return len(self.l)==0
        
    def top(self):
        if len(self.l)!=0:
            return self.l[-1]
        
class Queue:
    def __init__(self):
        self.stack_in = Stack()  
        self.stack_out = Stack() 

    def append(self, value):
        self.stack_in.push(value)

    def pop(self):
        if self.stack_out.isEmpty():  # Only transfer if stack_out is empty
            while not self.stack_in.isEmpty():
                self.stack_out.push(self.stack_in.pop())
        if not self.stack_out:  # If stack_out is still empty, queue is empty
            raise IndexError("dequeue from an empty queue")
        return self.stack_out.pop()
        

    # Check if the queue is empty  
    def is_empty(self):
        return self.stack_in.isEmpty() and self.stack_out.isEmpty()

    # Get the size of the queue
    def size(self):
        return len(self.stack_in.l) + len(self.stack_out.l)

class Compare:

    def comp1(self,O1,O2):
       return O1[0] < O2[0]
    
    def comp2(self,O1,O2):
        if O1[0]!=O2[0]:
            return O1[0] < O2[0]
        else:
            return O1[1] < O2[1]

class Heap:
    
    def __init__(self, init_array):
        
        self.he = init_array
        if len(init_array) == 5:
            self.comp = Compare.comp2
        else:
            self.comp = Compare.comp1
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


class Planner:
    def __init__(self, flights):
        """The Planner
        Args:
            flights (List[Flight]): A list of information of all the flights (objects of class Flight)
        """

        self.flights = flights
        self.no_cities = 0
        for j in flights:
            if j.start_city > self.no_cities:
                self.no_cities = j.start_city
            if j.end_city > self.no_cities:
                self.no_cities = j.end_city
        self.adj_list = [[] for _ in range(self.no_cities + 1)]
        for plane in flights:
            self.adj_list[plane.start_city].append(plane)
        self.min_time = [(1e9,1e9) for _ in range(self.no_cities + 1)]
        for plane in flights:
            self.min_time[plane.start_city] = (min(plane.departure_time,self.min_time[plane.start_city][0]),float('inf'))
        for k in self.adj_list:
            k.sort(key= lambda x:x.departure_time)

    def least_flights_earliest_route(self, start_city, end_city, t1, t2):

        vis = [False for _ in range(len(self.flights) + 1)]
        qu = Queue()
        time_array =  [(float('inf'), float('inf')) for _ in range(self.no_cities + 1)]
        time_array[start_city] = (0,0) 
        last_flight = [None for _ in range(len(self.flights) + 1)]
        end_flight  = None

        for j in self.adj_list[start_city]:
            if t1 <=j.departure_time and j.arrival_time <=t2:
                if j.end_city==end_city and j.arrival_time <time_array[j.end_city][1]:
                    end_flight = j
                    vis[j.flight_no] = True
                    time_array[j.end_city] = (1,j.arrival_time)
                    
                else:
                    qu.append((j,1))
                    time_array[j.end_city] = (1,j.arrival_time)
                    vis[j.flight_no] = True
                
        
        while qu:
            
            curr = qu.pop()
            if curr==-1:
                break
            curr_fli = curr[0]
            curr_city = curr[0].end_city
            
            curr_time = curr[0].arrival_time
            curr_count = time_array[curr_city][0]
            
            
            for k in self.adj_list[curr_city]:
                if k.end_city == end_city and (k.departure_time >= t1 and k.arrival_time <=t2) and k.departure_time >= curr_time+20 and curr_count+1 <= time_array[end_city][0] and vis[k.flight_no]==False:
                   
                    if curr_count+1 == time_array[end_city][0]:
                        if k.arrival_time <= time_array[end_city][1]:
                            end_flight = k
                            vis[k.flight_no] = True
                            time_array[end_city]=(curr_count+1,k.arrival_time)
                            last_flight[k.flight_no] = curr_fli
                    
                    elif curr_count+1 < time_array[end_city][0]:
                        end_flight = k
                        vis[k.flight_no] = True
                        time_array[end_city]=(curr_count+1,k.arrival_time)
                        last_flight[k.flight_no] = curr_fli

                    else:
                        break


                elif k.end_city!=end_city:

                    
                    if  k.departure_time >= curr_fli.arrival_time+20 and k.arrival_time<= t2 and vis[k.flight_no]==False:
                        
                        
                            
                                vis[k.flight_no] = True
                                qu.append((k,curr_count+1))
                                time_array[k.end_city] = (curr_count+1,k.arrival_time)
                                last_flight[k.flight_no] = curr_fli

                         
        
        route = []
        j = end_flight
        first_city = None
        while j!=None:
            first_city = j.start_city
            route.append(j)
            j = last_flight[j.flight_no]

        if first_city == start_city:
            return route[::-1]
        else:
            return []


        

    def cheapest_route(self, start_city, end_city, t1, t2):
        

        dist_arr = [(float('inf')) for _ in range(self.no_cities + 1)]
        last_flight = [(None) for _ in range(len(self.flights)+1)]
        dist_arr[start_city] = 0
        pq=Heap([(0,start_city,None)])
        end_flight = None

        while pq.heapsize!=0:

            curr = pq.extract()
            curr_fare = curr[0]
            

            for flight in self.adj_list[curr[1]]:
                new_fare = curr_fare + flight.fare
                a = True
                if curr[2]!=None:
                    if curr[2].arrival_time + 20 > flight.departure_time:
                        a = False

                b = False
                if flight.arrival_time + 20 < max(self.min_time[flight.end_city][0],self.min_time[flight.end_city][1]):
                    b = True

                if new_fare < dist_arr[flight.end_city] or (flight.arrival_time+20<=self.min_time[flight.end_city][0] and  a):
                    
                    if curr[2]==None:
                        time = t1-20
                    else:
                        time = curr[2].arrival_time

                    if flight.end_city == end_city and new_fare < dist_arr[flight.end_city] and flight.arrival_time<=t2 and flight.departure_time >= time + 20 :
                        end_flight = flight
                        dist_arr[flight.end_city] = new_fare
                        last_flight[end_flight.flight_no] = curr[2]
                    
                    elif flight.end_city == end_city:
                        continue

                    else:
                        if curr[2]!=None:
                            if flight.arrival_time + 20 <= self.min_time[flight.end_city][0] and flight.departure_time>=t1 and flight.arrival_time<=t2 and curr[2].arrival_time + 20 <= flight.departure_time :
                                self.min_time[flight.end_city] = (0,0)
                                pq.insert((new_fare,flight.end_city,flight))
                                dist_arr[flight.end_city] = new_fare
                                last_flight[flight.flight_no] = curr[2]
                        
                            elif curr[2].arrival_time + 20 <= flight.departure_time and flight.departure_time>=t1 and flight.arrival_time<=t2:
                                #self.min_time[flight.end_city] = (self.min_time[flight.end_city][0],flight.arrival_time)
                                pq.insert((new_fare,flight.end_city,flight))
                                dist_arr[flight.end_city] = new_fare
                                last_flight[flight.flight_no] = curr[2]

                        else:
                            if flight.arrival_time + 20 <= self.min_time[flight.end_city][0] and flight.departure_time>=t1 and flight.arrival_time<=t2  :
                                self.min_time[flight.end_city] = (0,0)
                                pq.insert((new_fare,flight.end_city,flight))
                                dist_arr[flight.end_city] = new_fare
                                last_flight[flight.flight_no] = curr[2]
                        
                            elif flight.departure_time>=t1 and flight.arrival_time<=t2:
                                #self.min_time[flight.end_city] = (self.min_time[flight.end_city][0],flight.arrival_time)
                                pq.insert((new_fare,flight.end_city,flight))
                                dist_arr[flight.end_city] = new_fare
                                last_flight[flight.flight_no] = curr[2]



        first_city = None
        route  = []
        while end_flight!=None:
            route.append(end_flight)
            first_city = end_flight.start_city
            end_flight = last_flight[end_flight.flight_no]


        route.reverse()
        if first_city == start_city:
            return route
        else:
            return []

        
    def least_flights_cheapest_route(self, start_city, end_city, t1, t2):
        
        vis = [False for _ in range(len(self.flights) + 1)]
        h = Heap([(0,0,start_city,None,t1)])
        
        time_array =  [(float('inf'), float('inf')) for _ in range(self.no_cities + 1)]
        time_array[start_city] = (0,0)  #(ct,fare,lastflight)
        last_flight = [None for _ in range(len(self.flights) + 1)]
        end_flight  = None

        
        while h.heapsize!=0:
            curr = h.extract()
            if curr==-1:
                break
            curr_fli = curr[3]
            if curr_fli:
                curr_city = curr_fli.end_city
            else:
                curr_city = start_city
            curr_fare = curr[1]
            curr_count = curr[0]
            if curr_fli:
                curr_time = curr_fli.arrival_time
            else:
                curr_time = t1-20


            
            for k in self.adj_list[curr_city]:

                if k.end_city == end_city and (k.departure_time >= t1 and k.arrival_time <=t2) and k.departure_time>=curr_time+20 and  vis[k.flight_no]==False and curr_count+1 <= time_array[end_city][0]:
                    
                    if curr_count+1 == time_array[end_city][0]:
                        if k.fare + curr_fare <= time_array[end_city][1]:
                            vis[k.flight_no] = True
                            time_array[end_city]=(curr_count+1,k.fare + curr_fare)
                            last_flight[k.flight_no] = curr_fli
                            end_flight = k
                       
                    elif curr_count + 1 < time_array[end_city][0]:
                        vis[k.flight_no] = True
                        time_array[end_city]=(curr_count+1,k.fare + curr_fare)
                        last_flight[k.flight_no] = curr_fli
                        end_flight = k
                    
                    else:
                        break
                    

                else:
                
                    if  k.departure_time >= curr_time+20 and k.arrival_time <= t2 and vis[k.flight_no]==False:
                        if curr_count+1 <= time_array[k.end_city][0]:
                                                                 
                                if k.fare+curr_fare < time_array[k.end_city][1]:
                                        vis[k.flight_no] = True
                                        h.insert((curr_count+1,k.fare+curr_fare,k.end_city,k,k.arrival_time))
                                        time_array[k.end_city] = (curr_count+1,k.fare+curr_fare)
                                        last_flight[k.flight_no] = curr_fli

                        elif curr_count+1 < time_array[k.end_city][0] :
                            if k.fare+curr_fare < time_array[k.end_city][1]:
                                        vis[k.flight_no] = True
                                        h.insert((curr_count+1,k.fare+curr_fare,k.end_city,k,k.arrival_time))
                                        time_array[k.end_city] = (curr_count+1,k.fare+curr_fare)
                                        last_flight[k.flight_no] = curr_fli

                        
        route = []
        j = end_flight
        first_city = None
        while j!=None:
            
            route.append(j)
            first_city = j.start_city
            j = last_flight[j.flight_no]
        
        if first_city == start_city:
            return route[::-1]
        else:
            return []
       