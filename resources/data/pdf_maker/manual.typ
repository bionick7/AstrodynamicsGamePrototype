#let get_label = x => x.text.trim("[[").trim("]]").trim(("+"))

#let get_label_name(x) = {
  let s = get_label(x)
  s.trim(("shpmod_"))
   .trim(("shp_"))
   .replace("_", " ")
}

// Concepts
= Concepts
#show regex("\\[\\[.*?\\]\\]"): x => link(label(get_label(x)))[_#get_label_name(x)_]
#let concepts = yaml("../concepts.yaml")
#for (name, body) in concepts [
  == #name #label(name)
  #body
]

#let entry = content => box(
  stroke: 1pt, 
  inset: 1em, 
  radius: 3pt,
  width: 100%,
  content)

// Resources
#pagebreak()
= Resources
#let show_resource(name, rsc) = {
  if rsc.at("hidden", default: "n") != "y" {entry[
    == #name #label(name)
    #rsc.at("description")
  ]}
}

#let resources = yaml("../resources.yaml")
#columns(2)[
  #for (name, rsc) in resources [
    #show_resource(name, rsc)
  ]
]

// Ship modules
#pagebreak()
= Ship modules
#let show_module(mod) = {
  if mod.at("hidden", default: "n") != "y" {entry[
    == #mod.at("name") #label(mod.at("id"))
    #mod.at("description")
  ]}
}
#columns(2)[
  #let modules = yaml("../ship_modules.yaml").at("shipmodules")
  #modules.map(show_module).join()
]


// Ship classes
#pagebreak()
= Ship classes
#let show_class(class) = {
  if class.at("hidden", default: "n") != "y" {entry[
    == #class.at("name") #label(class.at("id"))
    #class.at("description")
    #table(columns: 3, align: right,
      [Capacity], link("Δv")[Δv], [$I_(s p)$],
      str(class.at("capacity")), str(class.at("dv")), str(class.at("Isp")), 
    )
  ]}
}

#let classes = yaml("../ship_classes.yaml").at("ship_classes")
#classes.map(show_class).join()
