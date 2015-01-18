for start in xrange(1, 12):
    print "%i,%i,%i,%i,%i," % ((start)/8, (start+12)/8, (start+24)/8, (start+36)/8, (start+48)/8)
    print "%i,%i,%i,%i,%i," % ((start)%8, (start+12)%8, (start+24)%8, (start+36)%8, (start+48)%8)
    print 
