class Bin:
    def __init__(self, bin_id, capacity):
        self.bin_id  = bin_id
        self.capacity = capacity
        self.l=[]
        

    def add_object(self, object):
        # Implement logic to add an object to this bin
        if self.capacity >= object.size:
            
            self.capacity -= object.size
        else:
            raise NoBinFoundException
            
    
