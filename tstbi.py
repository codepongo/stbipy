import stbi

im = stbi.open('t.jpg')
im.save('t.png')
im.save('t.bmp', 'bmp')
im.crop(100, 100, 400, 400).save('t-resize.png')
im.gray().save('t-gray.png')
print 'input any key to clean the result...'
raw_input()
import os
os.remove('t.png')
os.remove('t.bmp')
os.remove('t-resize.png')
os.remove('t-gray.png')
