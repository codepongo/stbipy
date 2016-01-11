import stbi

im = stbi.open('t.jpg')
for y in range(2):
    for x in range(4):
        left = 5 + (67 + 5) * x;
        top = 41 + (67 + 5) * y;
        n = '%d-%d.png' % (x, y)
        im.crop(left, top, left+67, top+67).save(n)
        #perceptual hash
        im = stbi.open(n)
        t = '%d-%d-resize.png' % (x, y)
        im.resize(8,8).save(t)
        m = '%d-%d-gray.png' % (x, y)
        im.gray().save(m)
        tm = '%d-%d-resize-gray.png' % (x, y)
        stbi.open(t).gray().save(tm)
        im = stbi.open(tm)
        sum = 0
        for b in im.data:
            sum += b
        avg = sum / len(im.data)
        fingerprint = ''
        for b in im.data:
            if b > avg:
                fingerprint += '1'
            else:
                fingerprint += '0'
        print fingerprint
        print int(fingerprint, 2)
        break
    break
