def get_sigini_values(sigini_input):
    input = open(sigini_input,"r")
    lines = input.readlines()
    lc = lines[1].split(",")
    ltc = lines[2].split(",")
    input.close()
    return lc,ltc
    


def calculix_postprocess(frd_input) :
    input = open(frd_input,"r")
    nodes_x = []
    nodes_y = []
    nodes_z = []
    disp_x = []
    disp_y = []
    disp_z = []
    displaced_nodes_x = []
    displaced_nodes_y = []
    displaced_nodes_z = []

    disp_found = False
    nodes_found = True
    while True:
      line=input.readline()
      if not line: break
      #first lets extract the node and coordinate information from the results file
      if nodes_found and (line[1:3] == "-1"):
        nodes_x.append(float(line[13:25]))
        nodes_y.append(float(line[25:37]))
        nodes_z.append(float(line[37:49]))
      #Check if we found displacement section
      if line[5:9] == "DISP":
         disp_found = True
      #we found a displacement line in the frd file
      if disp_found and (line[1:3] == "-1"):
         disp_x.append(float(line[13:25]))
         disp_y.append(float(line[25:37]))
         disp_z.append(float(line[37:49]))
      #Check for the end of a section   
      if line[1:3] == "-3":
         #the section with the displacements and the nodes ended
         disp_found = False  
         nodes_found = False
      
    input.close()

    #Now we are able to generate the new bounding box based on the data in nodes and displacement
    for i in range(0,len(nodes_x)):
        displaced_nodes_x.append(nodes_x[i] + disp_x[i])
        displaced_nodes_y.append(nodes_y[i] + disp_y[i])
        displaced_nodes_z.append(nodes_z[i] + disp_z[i])

    #Now we can generate the required output
    #Bounding Box Volume calculation
    bbox_orig = (max(nodes_x)-min(nodes_x))*(max(nodes_y)-min(nodes_y))*(max(nodes_z)-min(nodes_z))
    bbox_distorted = (max(displaced_nodes_x)-min(displaced_nodes_x))*(max(displaced_nodes_y)-min(displaced_nodes_y))*(max(displaced_nodes_z)-min(displaced_nodes_z))
    relationship = bbox_orig/bbox_distorted*100
    max_disp_x = max(disp_x) 
    min_disp_x = min(disp_x)
    max_disp_y = max(disp_y)
    min_disp_y = min(disp_y)
    max_disp_z = max(disp_z)
    min_disp_z = min(disp_z)
    return bbox_orig,bbox_distorted,relationship,max_disp_x,min_disp_x,max_disp_y,min_disp_y,max_disp_z,min_disp_z



