# Clone of MacOS app "Grapher"
with raylib  
---

Initially this is for practicing rendering and interaction, but the hard part is in parsing expressions, and as always, memory management.  

I used "immediate mode" ui rendering, and reused some code from `c_simp_interp` project.  

Can't figure out a way to apply antialiasing to raylib `RenderTexture` though, so the graphics is bad. Text rendering quality is also bad.  

todos:  
- drag to adjust input area and sidebar sizes
- text selection
- math style formula rendering and input (ambitious!)

