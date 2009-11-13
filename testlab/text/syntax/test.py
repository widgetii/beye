#
# this file was created for testing purposes only
#


import os
os.environ['OS_MODULES'] = "os:print" # Single line comment

import somemodule
somemodule.require('1.2.3')
import othermodule
import thirdmodule

class Application:
  '''
  Python allows using multiline 
  constants. It doesn't *really* have anything other for that.
  '''
  def destroy(self, widget, data=None):
    somemodule.main_quit()
    
  def __init__(self):
    self.win = thirdmodule.Window(WINDOW_TOPLEVEL)
    self.win.connect("destroy", self.destroy)
    self.win.set_border_width(10)
  
  def main(self):
    othermodule.main()
    
class AccessorApp(AccessibleApp):
  """
  Python support other kind
  of multilne string too. But out parser distinguis nestes " characters
  """
  def __init__(self):
    AccessibleApp.__init__(self)
    self.accessible_registry = get_object("MyObject:1.0", "Registry")
    
# Utility methods for accessing components. If there gets to be too many, should
# go in a separate module.
# 
def find_comp_in_component(comp, comp_name, comp_role):
  'Python is depended on number of leading spaces and support "single-quoted" lines'
  for i in range(comp.childCount):
    child = comp.getChildAtIndex(i)
    if child.getRoleName() == comp_role:
      if child.name == comp_name:
        return child
    else:
      child_in_subtree = find_comp_in_component(child, comp_name, comp_role)
      if child_in_subtree:
        return child_in_subtree
  return None
      
    
def find_comp(desktop, application_name, comp_name, comp_role):
  "That's character constant too"
  for i in range(desktop.childCount):
    app = desktop.getChildAtIndex(i)
    if app.name == application_name:
      return find_comp_in_component(app, comp_name, comp_role)
  return None
