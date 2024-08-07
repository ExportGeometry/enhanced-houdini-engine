# Enhanced Houdini Engine for Unreal

This plugin collects the tools I've developed to extend the base [Houdini Engine](https://github.com/sideeffects/HoudiniEngineForUnreal) functionality.



## Demo Video

* [HBSG Demo (YouTube)](https://www.youtube.com/watch?v=Tbzxm4FqhpI)



## Supported Versions

This plugin currently supports:

* UE 5.4
* Houdini 20.0.724
* [Houdini Engine for Unreal 2.1.4](https://github.com/sideeffects/HoudiniEngineForUnreal/releases/tag/v2.1.4)



## Installation

1) Make sure that you have installed a supported version of **Unreal Engine** and **Houdini**.
   1) See the "Supported Versions" section above.
2) Create a C++ UE project.
3) Install a supported version of the **Houdini Engine for Unreal** plugin
   1) See the "Supported Versions" section above.
4) Add the EnhancedHoudiniEngine plugin to your UE project's Plugins/ folder.
5) Regenerate visual studio project files.
   1) Close your IDE, go to your .uproject file, right click, and select "generate visual studio project files".
6) Build and run your project.



## Tools

### Houdini Build Sequence Graph

This tool adds a custom graph that can be used to cook groups of Houdini assets in sequence. Additionally, you can use it to trigger arbitrary native UE actions, such as calling a console command. The current feature set is fairly limited, but it provides a framework so that adding new nodes is fairly easy.



#### HBSG Usage

This system is comprised of two main components:

* Houdini Build Sequence Graph (HBSG):  This is the custom graph where you define your cook rules.
* Houdini Build Manager: This is a special actor you place in your scene so that your HBSG knows where to look for HDA actors.



To use HBSG:

1) Create an empty HoudiniBuildSequenceGraph
   1) In the content browser, **right click -> Houdini Engine Custom -> Houdini Build Sequence Graph**
2) Wire up whatever nodes you want to execute in your graph.
   1) See the **Node Bible** below for more info on HBSG nodes.
3) Create a HoudiniBuildManager
   1) In the content browser, **right click -> Houdini Engine Custom -> Houdini Build Manager**
   2) Set the *Sequence Graph* class property to the graph you just created.
4) Drag the HoudiniBuildManager asset you just created into the scene where you want to trigger HDA asset actions.
5) Select your build manager from the sequence graph manager selector dropdown.
   1) In the toolbar above your sequence graph, you should see a dropdown widget. If you click the dropdown, you should now see your build manager listed.
6) Click the run button in the toolbar above your sequence graph to execute the graph.



#### HBSG Node Bible

##### Houdini Nodes

**Cook HDA**

This node allows you to cook a collection of HDAs. You can specify one or more *Asset Types* to cook all HDA assets in your scene of the listed types. You can also supply one or more *Actor Tags* to cook specific HDA actors in your scene using the standard Unreal Engine actor tag property.



**Rebuild HDA**

This node works the same as **Cook HDA**, but performs a rebuild instead. You can specify one or more *Asset Types* to rebuild all HDA assets in your scene of the listed types. You can also supply one or more *Actor Tags* to rebuild specific HDA actors in your scene using the standard Unreal Engine actor tag property.



##### Landscape Nodes

**Clear Landscape Layers**

This node allows you to clear a set of landscape paint layers. For each edit layer added to this node's *Edit Layers* parameter, it clears all paint layers added to this node's *Paint Layers* parameter.



**Flush Grass Cache**

Convenience node for refreshing UE landscape grass. This node is the same as triggering a **Console Command** node with the string `grass.FlushCache` or `grass.FlushCachePIE` (depending if bFlushLandscapeGrassData is true or false).



##### Utility Nodes

**Console Command**

Allows you to trigger any arbitrary console command.

