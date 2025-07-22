def process_nested_parent_str(attr_str):
    """
    The first letter should be a parenthesis
    input string: "(1,4,(5,6),7)"
    output: tuple (1,4,(4,6),7)
    """
    params = []
    agg_scope_level = 0
    current_param = ""
    for i, ch in enumerate(attr_str):
        if ch == ",":
            params.append(current_param)
            current_param = ""
        elif ch == "(":
            agg_scope_level += 1
        elif ch == ")":
            agg_scope_level = 0
        elif agg_scope_level == 0:
            current_param += ch
    return params


def process_nested_parent_str2(attr_str, idx=0):
    """
    The first letter should be a parenthesis
    input string: "(1,4,(5,6),7)"
    output: ['1','4',['5','6'],'7']
    """
    # print 'Entering function with string %s'%(attr_str)
    params = []
    current_param = ""
    k = 0
    while k < len(attr_str):
        # print 'k in this function:%i'%k
        ch = attr_str[k]
        k += 1
        if ch == ",":
            # print "Add param:",current_param
            params.append(current_param)
            current_param = ""
        elif ch == "(":
            nv = attr_str[k:]
            # print "Up one level parenthesis:%s"%(nv)
            current_param, progress = process_nested_parent_str2(nv)
            # print "Adding the list returned from nested",current_param
            params.append(current_param)
            current_param = ""
            k += progress + 1
        elif ch == ")":
            # print "Down one level parenthesis: %i characters parsed"%k
            params.append(current_param)
            # print "Current params:",params#k -= acc-2
            return params, k
        else:
            current_param += ch
        # print "Ch:",ch
        # print "k:",k

        # raw_input("")
        # idx += 1

    params.append(current_param)
    return params, k


# print process_nested_parent_str2('1,2,3,4,5,6')
# idx=0
# print process_nested_parent_str2("'A','B','C'")
print(process_nested_parent_str2("'A'")[0])
print(process_nested_parent_str2("30.0,0.0,5.0")[0])
print(process_nested_parent_str2("(Thomas)")[0])
print(process_nested_parent_str2("Thomas, Paviot, ouais")[0])
print(process_nested_parent_str2("1,2,(3,4,5),6,7,8")[0])
print(process_nested_parent_str2("(#9149,#9166),#9142,.T.")[0])
