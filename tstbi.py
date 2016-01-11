import stbi

im = stbi.open('t.jpg')
#im.save('t.png')
#im.save('t.bmp', 'bmp')
#im.crop(100, 100, 400, 400).save('t-resize.png')
#im.gray().save('t-gray.png')
d = im.data
sz = im.size
byte = 0
for pixel in xrange(sz[0] * sz[1]):
    d[byte+0] = 0xff - d[byte+0]
    d[byte+1] = 0xff - d[byte+1]
    d[byte+2] = 0xff - d[byte+2]
    byte += 4
im.data = d
im.save('t-invert-color.png')
#print 'input any key to clean the result...'
#raw_input()
#import os
#os.remove('t.png')
#os.remove('t.bmp')
#os.remove('t-resize.png')
#os.remove('t-gray.png')
#os.remove('t-red.png')

