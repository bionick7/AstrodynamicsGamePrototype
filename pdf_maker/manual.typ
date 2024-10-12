#set text(font: "Monospac821 BT")
#set page(number-align: center, numbering: "1/1")
#set par(justify: true)

#let DATA_FOLDER = "../resources/data/"

//#show heading.where(): it => align(center, text(font: "Berlin Sans FB", underline(it))) + v(5pt)
#show heading.where(level: 1): it => align(center, text(
  font: "Trebuchet MS", 
  size: 24pt,
  it)) + v(5pt) + line(length: 100%)
#show heading.where(level: 2): it => align(center, underline(it)) + v(5pt)

// Display library

#let get_label(x) = {
  x.text.trim("[[")
        .trim("]]")
        .trim(("+"))
}

#let get_label_name(x) = {
  get_label(x).trim(("shpmod_"))
              .trim(("shp_"))
              .replace("_", " ")
}

#let scientific_notation(x, suffix) = {
  if (x == 0) [$0 #suffix$]
  else {
    let exp = calc.floor(calc.log(calc.abs(x), base:10.0))
    [$  #calc.round(x / calc.pow(10.0, exp), digits: 2) times 10^#exp #suffix$]
  }
}

#outline(depth: 1)

// Concepts
= Concepts
#let concepts = yaml(DATA_FOLDER + "concepts.yaml")
#show regex("\\[\\[.*?\\]\\]"): x => {
  let id = get_label(x)
  box(fill: yellow,
  link(
    label(id), 
    concepts.at(id, default: (title: get_label_name(x))).title
  )
  )
}

#for (id, concept) in concepts [
  == #concept.title #label(id)
  #concept.body
]

#let entry = content => box(
  stroke: none, 
  inset: 1em, 
  radius: 3pt,
  width: 100%,
  content)

// Resources
#pagebreak()
= Resources

#let resources = yaml(DATA_FOLDER + "resources.yaml")
#columns(2)[
  #for (id, rsc) in resources {
    if rsc.at("hidden", default: "n") != "y" {entry[
      == #rsc.display_name #label(id)
      #rsc.description
    ]}
  }
]

// Ship modules
#pagebreak()
= Ship modules
#let show_module(mod) = entry[
  == #mod.at("name") #label(mod.at("id"))
  #mod.at("description")
]
#let modules = yaml(DATA_FOLDER + "ship_modules.yaml").at("shipmodules").filter(
  mod => mod.at("hidden", default: "n") != "y" )
#table(columns: 2, ..modules.map(show_module))

// Ship classes
#pagebreak()
= Ship classes
#let show_class(class) = {
  if class.at("hidden", default: "n") != "y" {entry[
    == #class.at("name") #label(class.at("id"))
    #class.at("description")
    #grid(columns: (70%, auto),
      image("images/ships/" + class.id + ".png"),
      h(5em) + align(center, table(columns: 2, align: right,
        [Capacity], $class.at("capacity") "Counts"$,
        link("Δv")[Δv], $class.at("dv") "km/s"$,
        [$I_(s p)$], $class.at("Isp") "km/s"$, 
      ))
    )
  ]}
}

#let classes = yaml(DATA_FOLDER + "ship_classes.yaml").at("ship_classes")
#classes.map(show_class).join()

// Planets
#pagebreak()
= Planets
.
#let show_planet(planet_data) = [
  #pagebreak()
  == #planet_data.name
  #h(2em)

  #let table_content
  #{
    let sma = planet_data.SMA
    let period = 2 * calc.pi * calc.sqrt(sma*sma*sma / 3.7931187e16)
    let period_D = calc.floor(period / 86400.0)
    let period_H = calc.floor((period - period_D*86400) / 3600)
    let has_atmosphere = planet_data.at("has_atmosphere", default:"n") == "y"
    table_content = (
      [Mass], scientific_notation(planet_data.mass, "kg"),
      [Atmosphere\ present?], if (has_atmosphere) [Yes] else [No],
    )
    if (planet_data.name != "Saturn") {
      table_content += (
        [Orbital\ Radius], scientific_notation(sma / 1000, "km"),
        [Orbital\ Period], $#period_D "D " #period_H "H "$,
      )
    }
    table_content += ([Dominant\ organization], planet_data.at("manual_dominant_org"))
  }

  #set par(justify: false)
  #grid(columns: (60%, auto),
    align(center, image("images/planets/" + planet_data.name + ".png", width: 15em)),
    align(center, table(columns: 2, ..table_content))
  )
  #set par(justify: true)
  #planet_data.description
]

#{
  let ephimeris = yaml(DATA_FOLDER + "ephemeris.yaml")
  show_planet(ephimeris)
  for moon in ephimeris.satellites {
    show_planet(moon)
  }
}