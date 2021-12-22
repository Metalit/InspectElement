from os import environ
environ["KIVY_NO_CONSOLELOG"] = "1"

from client import start_client, stop_client

from kivy.app import App
from kivy.uix.widget import Widget
from kivy.properties import (
    NumericProperty, StringProperty, AliasProperty, ReferenceListProperty,
    ObjectProperty, ListProperty, DictProperty, BooleanProperty)
from kivy.clock import Clock
from kivy.uix.floatlayout import FloatLayout
from kivy.uix.stacklayout import StackLayout
from kivy.uix.anchorlayout import AnchorLayout
from kivy.uix.label import Label
from kivy.uix.textinput import TextInput

def makeMethodCell(parent, methodTyp, methodName, argTyps, argNames):
    cell = MethodCell()
    cell.add_widget(CompactLabel(text=methodTyp), index=len(cell.children))
    cell.add_widget(CompactLabel(text=methodName), index=len(cell.children))

    for typ, name in zip(argTyps, argNames):
        cell.add_widget(CompactLabel(text=typ), index=len(cell.children))
        cell.add_widget(CompactLabel(text=name), index=len(cell.children))
        cell.add_widget(FieldInput(), index=len(cell.children))

    parent.add_widget(cell)

class PaddingCell(AnchorLayout):
    padding_horizontal = NumericProperty(5)
    padding_vertical = NumericProperty(5)

    widget = None

    def callback_size(self, *_):
        self.widget.width = self.width - self.padding_horizontal * 2
        self.widget.height = self.height - self.padding_vertical * 2

    def add_widget(self, widget, index=0, canvas=None):
        if(self.widget):
            raise Exception("Only one widget for a padding cell")
        self.widget = widget
        widget.size_hint = (None, None)
        self.bind(size=self.callback_size)
        return super().add_widget(widget, index=index, canvas=canvas)

class CompactLabel(Label):
    pass

class FieldInput(TextInput):
    pass

class ExpandingStack(StackLayout):
    isVertical = True
    verticalSet = False

    def callback_size(self, *args):
        if self.verticalSet:
            if self.isVertical:
                height = 0
                for widget in self.children:
                    height += widget.height
                self.height = height
            else:
                width = 0
                for widget in self.children:
                    width += widget.width
                self.width = width

    def add_widget(self, widget, *args):
        if self.verticalSet:
            if self.isVertical:
                widget.size_hint_y = None
                self.height += widget.height
            else:
                widget.size_hint_x = None
                self.width += widget.width
        widget.bind(size=self.callback_size)
        return super().add_widget(widget, *args)
    
    def remove_widget(self, widget, *args):
        if self.verticalSet:
            if self.isVertical:
                self.height -= widget.height
            else:
                self.width -= widget.width
        widget.unbind(size=self.callback_size)
        return super().remove_widget(widget, *args)

    def ExpandingStack_setup(self, *args):
        self.isVertical = self.orientation[:2] == "tb" or self.orientation[:2] == "bt"
        self.verticalSet = True
        if self.isVertical:
            self.size_hint_y = None
            for widget in self.children:
                widget.size_hint_y = None
        else:
            self.size_hint_x = None
            for widget in self.children:
                widget.size_hint_x = None
        self.callback_size()

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        Clock.schedule_once(self.ExpandingStack_setup)

class MethodCell(ExpandingStack):
    fieldInputs = ListProperty(list())

class ClassDisplay(ExpandingStack):
    collapsed = BooleanProperty(False)
    content = ObjectProperty(None)

    def toggle_collapsed(self, *args):
        toggled_collapse = not self.collapsed
        self.ids.expand_button.text = "+" if toggled_collapse else "-"
        if toggled_collapse:
            self.remove_widget(self.content)
        else:
            self.add_widget(self.content)
        self.collapsed = toggled_collapse
        
    def add_widget(self, widget, *args):
        # forward all added widgets to content
        if self.content and widget is not self.content:
            return self.content.add_widget(widget, *args)
        return super().add_widget(widget, *args)
        
    def remove_widget(self, widget, *args):
        # forward all removed widgets to content
        if self.content and widget is not self.content:
            return self.content.remove_widget(widget, *args)
        return super().remove_widget(widget, *args)

    def ClassDisplay_setup(self, *args):
        self.ids.expand_button.bind(on_press=self.toggle_collapsed)
        self.ids.expand_button.text = "+" if self.collapsed else "-"
        if self.collapsed:
            self.remove_widget(self.content)

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        Clock.schedule_once(self.ClassDisplay_setup)

class MainWidget(FloatLayout):
    connected = False

    def connect(self):
        if self.connected:
            stop_client()
            self.ids.connect_button.text = "Connect"
            self.connected = False
        else:
            self.connected = start_client(self.ids.ip_input.text, self.receive_message)
            if self.connected:
                self.ids.connect_button.text = "Disconnect"
    
    def receive_message(self, message):
        print(message, end="")

class GUIApp(App):
    def build(self):
        return MainWidget()

    def on_stop(self):
        stop_client()
        super().on_stop()

if __name__ == '__main__':
    GUIApp().run()