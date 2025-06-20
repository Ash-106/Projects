from enum import Enum

class Color(Enum):
    BLUE = 1
    YELLOW = 2
    RED = 3
    GREEN = 4
    

class Object:
    def __init__(self, object_id, size, color,BIN_id):
        self.ID = object_id
        self.size = size
        self.color = color
        self.BIN_id = BIN_id
        self.left = None
        self.right = None
        self.height = 1
        