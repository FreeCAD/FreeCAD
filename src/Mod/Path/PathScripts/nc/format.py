import math

class Format:
    def __init__(self, number_of_decimal_places = 3, add_leading_zeros = 1, add_trailing_zeros = False, dp_wanted = True, add_plus = False, no_minus = False, round_down = False):
        self.number_of_decimal_places = number_of_decimal_places
        self.add_leading_zeros = add_leading_zeros # fill the start of the number with zeros, so there are at least this number of digits before the decimal point
        self.add_trailing_zeros = add_trailing_zeros # fill the end of the number with zeros, as defined by "number_of_decimal_places"
        self.dp_wanted = dp_wanted
        self.add_plus = add_plus
        self.no_minus = no_minus
        self.round_down = round_down

    def string(self, number):
        if number == None:
            return 'None'
        f = float(number) * math.pow(10, self.number_of_decimal_places)
        s = str(f)
        
        if self.round_down == False:
            if f < 0: f = f - .5
            else: f = f + .5
            s = str(number)
            
        if math.fabs(f) < 1.0:
            s = '0'
            
        minus = False
        if s[0] == '-':
            minus = True
            if self.no_minus:
                s = s[1:]
        
        dot = s.find('.')
        if dot == -1:
            before_dp = s
            after_dp = ''
        else:
            before_dp = s[0:dot]
            after_dp = s[dot + 1: dot + 1 + self.number_of_decimal_places]
        
        before_dp = before_dp.zfill(self.add_leading_zeros)
        if self.add_trailing_zeros:
            for i in range(0, self.number_of_decimal_places - len(after_dp)):
                after_dp += '0'
        else:
            after_dp = after_dp.rstrip('0')
                 
        s = ''

        if minus == False:
            if self.add_plus == True:
                s += '+'
        s += before_dp
        if len(after_dp):
            if self.dp_wanted: s += '.'
            s += after_dp
            
        return s
    
class Address:
    def __init__(self, text, fmt = Format(), modal = True):
        self.text = text
        self.fmt = fmt
        self.modal = modal
        self.str = None
        self.previous = None
        
    def set(self, number):
        self.str = self.text + self.fmt.string(number)
        
    def write(self, writer):
        if self.str == None: return ''
        if self.modal:
            if self.str != self.previous:
                writer.write(self.str)
                self.previous = self.str            
        else:
            writer.write(self.str)
        self.str = None
    
class AddressPlusMinus(Address):
    def __init__(self, text, fmt = Format(), modal = True):
        Address.__init__(self, text, fmt, modal)
        self.str2 = None
        self.previous2 = None
        
    def set(self, number, text_plus, text_minus):
        Address.set(self, number)
        if float(number) > 0.0:
            self.str2 = text_plus
        else:
            self.str2 = text_minus

    def write(self, writer):
        Address.write(self, writer)
        if self.str2 == None: return ''
        if self.modal:
            if self.str2 != self.previous2:
                writer.write(writer.SPACE())
                writer.write(self.str2)
                self.previous2 = self.str2            
        else:
            writer.write(writer.SPACE())
            writer.write(self.str2)
        self.str2 = None
