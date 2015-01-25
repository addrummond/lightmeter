for start in range(0, 12):
    print "%i,%i,%i,%i,%i," % ((start)//8, (start+13)//8, (start+26)//8, (start+39)//8, (start+52)//8)
    print "%i,%i,%i,%i,%i," % ((start)%8, (start+13)%8, (start+26)%8, (start+39)%8, (start+52)%8)
    print 
