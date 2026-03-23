from __future__ import print_function
# make the handler of the color field to call chicken_mcnuggets()
# instead of the default set handle_color() function
handler_registry['color'] = 'chicken_mcnuggets'

def chicken_mcnuggets():
  # print color.getValue().getValue()
  pass

# Initialize the color Packer (required of any property node that
# uses an SoColorPacker to set diffuse color or transparency:
colorPacker = SoColorPacker()
transpValue = floatp()

def doAction(action):
    global transpValue

    if not brightness.isIgnored() and not SoOverrideElement.getEmissiveColorOverride(action.getState()):
        emissiveColor = color.getValue() * brightness.getValue()
        # print 'doAction():', color.getValue().getValue()

        # Use the Lazy element to set emissive color. 
        # Note that this will not actually send the color to GL.       
        SoLazyElement.setEmissive(action.getState(), emissiveColor)

    # To send transparency we again check ignore flag and override element.
    if not transparency.isIgnored() and not SoOverrideElement.getTransparencyOverride(action.getState()):
        # keep a copy of the transparency that we are putting in the state:
        transpValue.assign(transparency.getValue())
     
        # The color packer must be provided when the transparency is set,
        # so that the transparency will be merged with current diffuse color
        # in the state:
        SoLazyElement.setTransparency(action.getState(), self, 1, transpValue, colorPacker)

def GLRender(action):
    action.setTransparencyType(SoGLRenderAction.SORTED_OBJECT_BLEND)
    doAction(action)

def callback(action):
    doAction(action)

wa = SoWriteAction()
wa.apply(self)

print(handler_registry)

print('== Glow script loaded ==')
