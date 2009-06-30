import nc

class CreatorHpgl2d(nc.Creator):
    def init(self): 
        nc.Creator.init(self) 

    def program_begin(self, id, name=''):
        self.write('IN;\n')
        self.write('VS32,1;\n')
        self.write('VS32,2;\n')
        self.write('VS32,3;\n')
        self.write('VS32,4;\n')
        self.write('VS32,5;\n')
        self.write('VS32,6;\n')
        self.write('VS32,7;\n')
        self.write('VS32,8;\n')

nc.creator = CreatorHpgl2d()
