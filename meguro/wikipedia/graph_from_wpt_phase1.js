/*
{
  common_name: [<concept>,...],
  descriptions: [{text:<text>, source_name:<source>, source_URL:<URL>, source_updated_at:<date or null>}, ...]
  edges: {
    <connected concept>: {
      descriptors: [
        {relationship_category:<category>, relationship_subcategory:<subcategory>, snippet:<custom snippet or empty string>, source_name:<source_name>, source_URL:<URL>, distance:<distance>, added_at:<date>, updated_at:<date>, source_updated_at:<date or null>},
        ...
      ],
      distance: <minimum of above distances>,
      added_at: <date>,
      updated_at: <date>
    },
    ...
  },
  prominence: <distance (based on number of inlinks or such)>,
  aliases: [ <alias>, .... ],
  web_sites: {crunchbase: <URL>, home_page:<URL>, twitter: <URL>, ...},
  locations: [{latitude: <latitude>, longitude: <longitude>, ...],
  added_at: <date>,
  last_updates: {<source_name>:<date>, ...}
} 
 */

var ATTRIBUTE_RULES = {
  "infobox actor": {
    "occupation": {
      "reverse_distance": 60,
      "subcategory": "OCCUPATION",
      "category": "PERSON",
      "template": "[[T]] worked as a [[X]]",
      "distance": 20
    },
    "domestic_partner": {
      "reverse_distance": 10,
      "subcategory": "PARTNER",
      "category": "GENEOLOGICAL",
      "template": "[[T]] lives with [[X]]",
      "distance": 10
    },
    "birthplace": {
      "reverse_distance": 70,
      "subcategory": "BIRTH",
      "category": "PERSON",
      "template": "[[T]] was born in [[X]]",
      "distance": 30
    },
    "spouse": {
      "reverse_distance": 10,
      "subcategory": "SPOUSE",
      "category": "GENEOLOGICAL",
      "template": "[[T]] and [[X]] are married",
      "distance": 10
    }
  },
  "infobox film": {
    "studio": {
      "reverse_distance": 30,
      "subcategory": "LOCATION",
      "category": "FILM",
      "template": "[[T]] was filmed in [[X]]",
      "distance": 10
    },
    "writer": {
      "reverse_distance": 10,
      "subcategory": "WRITER",
      "category": "FILM",
      "template": "[[X]] wrote [[T]]",
      "distance": 10
    },
    "director": {
      "reverse_distance": 10,
      "subcategory": "DIRECTOR",
      "category": "FILM",
      "template": "[[X]] directed [[T]]",
      "distance": 10
    },
    "editing": {
      "reverse_distance": 10,
      "subcategory": "EDITOR",
      "category": "FILM",
      "template": "[[X]] edited [[T]]",
      "distance": 10
    },
    "cinematography": {
      "reverse_distance": 10,
      "subcategory": "CINEMATOGRAPHER",
      "category": "FILM",
      "template": "[[X]] did the cinematography for [[T]]",
      "distance": 10
    },
    "music": {
      "reverse_distance": 10,
      "subcategory": "COMPOSER",
      "category": "FILM",
      "template": "[[X]] composed the music for [[T]]",
      "distance": 10
    },
    "starring": {
      "reverse_distance": 10,
      "subcategory": "STAR",
      "category": "FILM",
      "template": "[[X]] appeared in [[T]]",
      "distance": 10
    },
    "distributor": {
      "reverse_distance": 30,
      "subcategory": "DISTRIBUTOR",
      "category": "FILM",
      "template": "[[T]] was distributed by [[X]]",
      "distance": 30
    },
    "producer": {
      "reverse_distance": 10,
      "subcategory": "PRODUCER",
      "category": "FILM",
      "template": "[[X]] produced [[T]]",
      "distance": 10
    }
  },
  "infobox single": {
    "artist": {
      "reverse_distance": 10,
      "subcategory": "ARTIST",
      "category": "MUSIC",
      "template": "[[T]] is a single by [[X]]",
      "distance": 10
    },
    "writer": {
      "reverse_distance": 10,
      "subcategory": "LYRICIST",
      "category": "MUSIC",
      "template": "[[X]] wrote [[T]]",
      "distance": 30
    },
    "format": {
      "reverse_distance": 60,
      "subcategory": "FORMAT",
      "category": "MUSIC",
      "template": "[[T]] was released in [[X]] format",
      "distance": 30
    },
    "genre": {
      "reverse_distance": 70,
      "subcategory": "GENRE",
      "category": "MUSIC",
      "template": "[[T]] is in the genre of [[X]]",
      "distance": 20
    },
    "from album": {
      "reverse_distance": 10,
      "subcategory": "ALBUM",
      "category": "MUSIC",
      "template": "[[T]] is a single from the album [[X]]",
      "distance": 10
    },
    "label": {
      "reverse_distance": 50,
      "subcategory": "LABEL",
      "category": "MUSIC",
      "template": "[[T]] was released by [[X]]",
      "distance": 30
    }
  },
  "infobox broadcast": {
    "city": {
      "reverse_distance": 60,
      "subcategory": "LOCATION",
      "category": "BROADCAST MEDIA",
      "template": "[[T]] broadcasts in [[X]]",
      "distance": 50
    },
    "affiliations": {
      "reverse_distance": 50,
      "subcategory": "AFFILIATION",
      "category": "BROADCAST MEDIA",
      "template": "[[T]] is affiliated with [[X]]",
      "distance": 30
    },
    "licensee": {
      "reverse_distance": 40,
      "category": "BROADCAST MEDIA",
      "template": "[[X]] licenses [[T]]",
      "distance": 30
    },
    "sister_stations": {
      "reverse_distance": 40,
      "subcategory": "AFFILIATION",
      "category": "BROADCAST MEDIA",
      "template": "[[T]] is a twin station of [[X]]",
      "distance": 40
    },
    "owner": {
      "reverse_distance": 40,
      "subcategory": "INVESTMENT",
      "category": "BUSINESS",
      "template": "[[X]] owns [[T]]",
      "distance": 10
    },
    "former_affiliations": {
      "reverse_distance": 30,
      "subcategory": "AFFILIATION",
      "category": "BROADCAST MEDIA",
      "template": "[[X]] was affiliated with [[T]]",
      "distance": 30
    },
    "location": {
      "reverse_distance": 60,
      "subcategory": "LOCATION",
      "category": "BROADCAST MEDIA",
      "template": "[[T]]'s studio is located in [[X]]",
      "distance": 50
    }
  },
  "infobox website": {
    "author": {
      "reverse_distance": 10,
      "subcategory": "CREATOR",
      "category": "WEB",
      "template": "[[X]] created [[T]]",
      "distance": 10
    },
    "owner": {
      "reverse_distance": 30,
      "subcategory": "INVESTMENT",
      "category": "BUSINESS",
      "template": "[[X]] operates [[T]]",
      "distance": 30
    }
  },
  "infobox software": {
    "author": {
      "reverse_distance": 25,
      "subcategory": "DEVELOPER",
      "category": "TECHNOLOGY",
      "template": "[[X]] wrote [[y]]",
      "distance": 10
    },
    "developer": {
      "reverse_distance": 25,
      "subcategory": "DEVELOPER",
      "category": "TECHNOLOGY",
      "template": "[[X]] developed [[T]]",
      "distance": 10
    }
  },
  "infobox television": {
    "director": {
      "reverse_distance": 10,
      "subcategory": "DIRECTOR",
      "category": "BROADCAST MEDIA",
      "template": "[[T]] was directed by [[X]]",
      "distance": 10
    },
    "writer": {
      "reverse_distance": 10,
      "subcategory": "WRITER",
      "category": "BROADCAST MEDIA",
      "template": "[[T]] was written by [[X]]",
      "distance": 10
    },
    "creator": {
      "reverse_distance": 10,
      "subcategory": "CREATOR",
      "category": "BROADCAST MEDIA",
      "template": "[[T]] was created by [[X]]",
      "distance": 10
    },
    "opentheme": {
      "reverse_distance": 10,
      "subcategory": "MUSIC",
      "category": "BROADCAST MEDIA",
      "template": "[[T]]'s opening theme was [[X]]",
      "distance": 10
    },
    "narrated": {
      "reverse_distance": 10,
      "subcategory": "VOICE",
      "category": "BROADCAST MEDIA",
      "template": "[[T]] was narrated by [[X]]",
      "distance": 10
    },
    "voices": {
      "reverse_distance": 20,
      "subcategory": "VOICE",
      "category": "BROADCAST MEDIA",
      "template": "[[X]] was a voice on [[T]]",
      "distance": 20
    },
    "starring": {
      "reverse_distance": 10,
      "subcategory": "STAR",
      "category": "BROADCAST MEDIA",
      "template": "[[T]] starred [[X]]",
      "distance": 10
    },
    "endtheme": {
      "reverse_distance": 10,
      "subcategory": "MUSIC",
      "category": "BROADCAST MEDIA",
      "template": "[[T]]'s ending theme was [[X]]",
      "distance": 10
    },
    "channel": {
      "reverse_distance": 70,
      "subcategory": "CHANNEL",
      "category": "BROADCAST MEDIA",
      "template": "[[T]] was shown on [[X]]",
      "distance": 30
    },
    "genre": {
      "reverse_distance": 60,
      "subcategory": "GENRE",
      "category": "BROADCAST MEDIA",
      "template": "[[T]] belongs to the genre [[X]]",
      "distance": 20
    },
    "theme_music_composer": {
      "reverse_distance": 20,
      "subcategory": "MUSICIAN",
      "category": "BROADCAST MEDIA",
      "template": "[[T]]'s theme music was composed by [[X]]",
      "distance": 20
    },
    "location": {
      "reverse_distance": 70,
      "subcategory": "LOCATION",
      "category": "BROADCAST MEDIA",
      "template": "[[T]] was shot in [[X]]",
      "distance": 40
    },
    "judges": {
      "reverse_distance": 20,
      "subcategory": "STAR",
      "category": "BROADCAST MEDIA",
      "template": "[[X]] was a judge on [[T]]",
      "distance": 20
    }
  },
  "infobox newspaper": {
    "newseditor": {
      "reverse_distance": 10,
      "subcategory": "EDITOR",
      "category": "PRINT MEDIA",
      "template": "[[X]] is the news editor of [[T]]",
      "distance": 10
    },
    "chiefeditor": {
      "reverse_distance": 10,
      "subcategory": "EDITOR",
      "category": "PRINT MEDIA",
      "template": "[[X]] is the chief editor of [[T]]",
      "distance": 10
    },
    "sportseditor": {
      "reverse_distance": 10,
      "subcategory": "EDITOR",
      "category": "PRINT MEDIA",
      "template": "[[X]] is the sports editor of [[T]]",
      "distance": 10
    },
    "campuseditor": {
      "reverse_distance": 10,
      "subcategory": "EDITOR",
      "category": "PRINT MEDIA",
      "template": "[[X]] is the campus editor of [[T]]",
      "distance": 10
    },
    "photoeditor": {
      "reverse_distance": 10,
      "subcategory": "EDITOR",
      "category": "PRINT MEDIA",
      "template": "[[X]] is the photo editor of [[T]]",
      "distance": 10
    },
    "assoceditor": {
      "reverse_distance": 10,
      "subcategory": "EDITOR",
      "category": "PRINT MEDIA",
      "template": "[[X]] is the associate editor of [[T]]",
      "distance": 10
    },
    "sister newspapers": {
      "reverse_distance": 40,
      "subcategory": "RELATED",
      "category": "PRINT MEDIA",
      "template": "[[T]] and [[X]] are sister newspapers",
      "distance": 40
    },
    "staff": {
      "reverse_distance": 10,
      "subcategory": "WRITER",
      "category": "PRINT MEDIA",
      "template": "[[X]] is a writer at [[T]]",
      "distance": 10
    },
    "founder": {
      "reverse_distance": 10,
      "subcategory": "EMPLOYEE",
      "category": "BUSINESS",
      "template": "[[X]] founded [[T]]",
      "distance": 10
    },
    "campuschief": {
      "reverse_distance": 10,
      "subcategory": "EDITOR",
      "category": "PRINT MEDIA",
      "template": "[[X]] is the campus chief of [[T]]",
      "distance": 10
    },
    "managingeditordesign": {
      "reverse_distance": 10,
      "subcategory": "EDITOR",
      "category": "PRINT MEDIA",
      "template": "[[X]] is the managing design editor of [[T]]",
      "distance": 10
    },
    "editor": {
      "reverse_distance": 10,
      "subcategory": "EDITOR",
      "category": "PRINT MEDIA",
      "template": "[[X]] is the editor of [[T]]",
      "distance": 10
    },
    "publisher": {
      "reverse_distance": 10,
      "subcategory": "PUBLISHER",
      "category": "PRINT MEDIA",
      "template": "[[X]] is the publisher of [[T]]",
      "distance": 10
    },
    "opeditor": {
      "reverse_distance": 10,
      "subcategory": "EDITOR",
      "category": "PRINT MEDIA",
      "template": "[[X]] is the opinion editor of [[T]]",
      "distance": 10
    },
    "maneditor": {
      "reverse_distance": 10,
      "subcategory": "EDITOR",
      "category": "PRINT MEDIA",
      "template": "[[X]] is the managing editor of [[T]]",
      "distance": 10
    },
    "headquarters": {
      "reverse_distance": 50,
      "subcategory": "LOCATION",
      "category": "BUSINESS",
      "template": "[[T]]'s headquarters is in [[X]]",
      "distance": 50
    },
    "owner": {
      "reverse_distance": 20,
      "subcategory": "INVESTMENT",
      "category": "BUSINESS",
      "template": "[[X]] owns [[T]]",
      "distance": 20
    }
  },
  "infobox musical artist": {
    "occupation": {
      "reverse_distance": 70,
      "subcategory": "OCCUPATION",
      "category": "PERSON",
      "template": "[[T]] worked as a [[X]]",
      "distance": 30
    },
    "instrument": {
      "reverse_distance": 50,
      "subcategory": "PERSONAL_INSTRUMENT",
      "category": "MUSIC",
      "template": "[[T]] makes music with their [[X]]",
      "distance": 10
    },
    "past_members": {
      "reverse_distance": 10,
      "subcategory": "BAND_MEMBER",
      "category": "MUSIC",
      "template": "[[X]] was a member of [[T]]",
      "distance": 10
    },
    "notable_instruments": {
      "reverse_distance": 60,
      "subcategory": "PERSONAL_INSTRUMENT",
      "category": "MUSIC",
      "template": "[[T]] makes music with their [[X]]",
      "distance": 20
    },
    "associated_acts": {
      "reverse_distance": 50,
      "subcategory": "COLLABORATOR",
      "category": "MUSIC",
      "template": "[[T]] collaborated with [[X]]",
      "distance": 50
    },
    "origin": {
      "reverse_distance": 60,
      "subcategory": "BIRTH",
      "category": "PERSON",
      "template": "[[T]] was born in [[X]]",
      "distance": 30
    },
    "genre": {
      "reverse_distance": 60,
      "subcategory": "GENRE",
      "category": "MUSIC",
      "template": "[[T]]'s music belongs to the genre [[X]]",
      "distance": 30
    },
    "current_members": {
      "reverse_distance": 10,
      "subcategory": "BAND_MEMBER",
      "category": "MUSIC",
      "template": "[[X]] is a member of [[T]]",
      "distance": 10
    },
    "label": {
      "reverse_distance": 30,
      "subcategory": "LABEL",
      "category": "MUSIC",
      "template": "[[T]] was signed by [[X]]",
      "distance": 20
    },
    "spouse": {
      "reverse_distance": 10,
      "subcategory": "SPOUSE",
      "category": "GENEALOGICAL",
      "template": "[[T]] and [[X]] are married",
      "distance": 10
    }
  },
  "infobox television episode": {
    "director": {
      "reverse_distance": 20,
      "subcategory": "DIRECTOR",
      "category": "BROADCAST MEDIA",
      "template": "[[X]] directed [[T]]",
      "distance": 10
    },
    "writer": {
      "reverse_distance": 10,
      "subcategory": "WRITER",
      "category": "BROADCAST MEDIA",
      "template": "[[X]] wrote [[T]]",
      "distance": 10
    },
    "music": {
      "reverse_distance": 20,
      "subcategory": "MUSICIAN",
      "category": "BROADCAST MEDIA",
      "template": "[[X]] wrote the music for [[T]]",
      "distance": 20
    },
    "series": {
      "reverse_distance": 10,
      "subcategory": "SERIES",
      "category": "BROADCAST MEDIA",
      "template": "[[T]] was an episode of [[X]]",
      "distance": 10
    },
    "editor": {
      "reverse_distance": 10,
      "subcategory": "EDITOR",
      "category": "BROADCAST MEDIA",
      "template": "[[X]] edited [[T]]",
      "distance": 20
    },
    "producer": {
      "reverse_distance": 40,
      "subcategory": "PRODUCER",
      "category": "BROADCAST MEDIA",
      "template": "[[X]] produced [[T]]",
      "distance": 40
    },
    "photographer": {
      "reverse_distance": 10,
      "subcategory": "CREW",
      "category": "BROADCAST MEDIA",
      "template": "[[X]] was the director of photography for [[T]]",
      "distance": 30
    },
    "guests": {
      "reverse_distance": 50,
      "subcategory": "ACTOR",
      "category": "BROADCAST MEDIA",
      "template": "[[X]] made a guest appearance on [[T]]",
      "distance": 50
    }
  },
  "infobox magazine": {
    "company": {
      "reverse_distance": 40,
      "subcategory": "PUBLISHER",
      "category": "PRINT MEDIA",
      "template": "[[X]] publishes [[T]]",
      "distance": 40
    },
    "staff_writer": {
      "reverse_distance": 50,
      "subcategory": "WRITER",
      "category": "PRINT MEDIA",
      "template": "[[X]] writes for [[T]]",
      "distance": 20
    },
    "based": {
      "reverse_distance": 70,
      "subcategory": "LOCATION",
      "category": "PRINT MEDIA",
      "template": "[[T]] is based in [[X]]",
      "distance": 50
    },
    "previous_editor": {
      "reverse_distance": 10,
      "subcategory": "EDITOR",
      "category": "PRINT MEDIA",
      "template": "[[X]] was the editor of [[T]]",
      "distance": 10
    },
    "publisher": {
      "reverse_distance": 40,
      "subcategory": "PUBLISHER",
      "category": "PRINT MEDIA",
      "template": "[[T]] publishes [[X]]",
      "distance": 40
    },
    "editor": {
      "reverse_distance": 10,
      "subcategory": "EDITOR",
      "category": "PRINT MEDIA",
      "template": "[[X]] is the editor of [[T]]",
      "distance": 10
    }
  },
  "infobox book": {
    "author": {
      "reverse_distance": 10,
      "subcategory": "AUTHOR",
      "category": "LITERATURE",
      "template": "[[X]] wrote [[T]]",
      "distance": 10
    },
    "genre": {
      "reverse_distance": 60,
      "subcategory": "GENRE",
      "category": "LITERATURE",
      "template": "[[T]] belongs to the genre [[X]]",
      "distance": 50
    }
  },
  "infobox album": {
    "artist": {
      "reverse_distance": 10,
      "subcategory": "ARTIST",
      "category": "MUSIC",
      "template": "[[X]] recorded [[T]]",
      "distance": 10
    },
    "next album": {
      "reverse_distance": 40,
      "subcategory": "RELATED_ALBUM",
      "category": "MUSIC",
      "template": "[[X]] was the group's next album after [[T]]",
      "distance": 40
    },
    "last album": {
      "reverse_distance": 40,
      "subcategory": "RELATED_ALBUM",
      "category": "MUSIC",
      "template": "[[X]] was the group's last album before [[T]]",
      "distance": 40
    },
    "recorded": {
      "reverse_distance": 60,
      "subcategory": "RECORDING_YEAR",
      "category": "MUSIC",
      "template": "[[T]] was recorded during [[X]]",
      "distance": 20
    },
    "producer": {
      "reverse_distance": 50,
      "subcategory": "PRODUCER",
      "category": "MUSIC",
      "template": "[[X]] produced [[T]]",
      "distance": 40
    },
    "genre": {
      "reverse_distance": 70,
      "subcategory": "GENRE",
      "category": "MUSIC",
      "template": "[[T]] belongs to the genre [[X]]",
      "distance": 30
    },
    "released": {
      "reverse_distance": 60,
      "subcategory": "RELEASE_DATE",
      "category": "MUSIC",
      "template": "[[T]] was released on [[X]]",
      "distance": 20
    },
    "label": {
      "reverse_distance": 50,
      "subcategory": "LABEL",
      "category": "MUSIC",
      "template": "[[X]] released [[T]]",
      "distance": 30
    }
  },
  "infobox president": {
    "birth_place": {
      "reverse_distance": 50,
      "subcategory": "BIRTH",
      "category": "PERSON",
      "template": "[[T]] was born in [[X]]",
      "distance": 40
    },
    "occupation": {
      "reverse_distance": 80,
      "subcategory": "OCCUPATION",
      "category": "PERSON",
      "template": "[[T]] worked as a [[X]]",
      "distance": 30
    },
    "religion": {
      "reverse_distance": 70,
      "subcategory": "RELIGION",
      "category": "PERSON",
      "template": ["[[T]] is [[X]]", {r: "(ity)|(ism)|(lam)$", t: "[[T]]'s religion is [[X]]"}],
      "distance": 20
    },
    "spouse": {
      "reverse_distance": 10,
      "subcategory": "SPOUSE",
      "category": "GENEALOGICAL",
      "template": "[[T]] and [[X]] are married",
      "distance": 10
    }
  }
};

function page_is_ambiguous(title,page_json) {
  if (title.match(/\(disambiguation\)$/)) {
    return true;
  }
  for(var i=0, il=page_json.length; i < il; ++i) {
    var elem = page_json[i];
    if (typeof(elem) != 'string' && elem.type == 'template') {
      if (elem.title == 'disambig' || elem.title == 'disambiguation') {
        return true;
      }
    }
  }
  return false;
}

source_name = 'Wikipedia';

/* Lowercase and remove underscores to normalize template names */
function keyify(template_title) {
  return template_title.toLowerCase().replace("_"," ");
}

/* Do a replacement on the snippet */
function snippet_substitute(template,t,x) {
  if (typeof(template) != 'string') {
    var i;
    for (i=template.length-1; i > 0; --i) {
      var r = new RegExp(template[i].r);
      if (r.test(x)) {
        template = template[i].t;
        break;
      }
    }
    if (i == 0) {
      template = template[0];
    }
  }
  var res = template.replace("[[T]]",'[['+t+']]').replace("[[X]]",'[['+x+']]');
  if (!res.match(/[\.\?!]$/)) { res += '.'; }
  return res;
}


function do_top_level_field(in_value, out_hash, out_field) {
  if (in_value) {
    if (typeof(in_value) != 'string') {
      in_value = String(in_value);
    }
    if (in_value.match(/\S/)) {
      //out_hash[out_field] = clean_string(in_value);
      out_hash[out_field] = in_value;
    }
  }
}



/*
 * If a page is a redirect we want to emit an alias type
 */
function handle_redirect(key,redirect, output) {
  if (redirect.target) {
    Meguro.emit(redirect.target.replace(/#.*/,''), JSON.stringify({'alias':key, 'fr':'r', 'distance':1})); //'r' stands for redirect, so we know we got this alias from redirect
    return true;
  }
  return false;
}

/*
 * Pull all of the internal links out of the param value of an infobox
 */
function pull_internal_links_out_of_param_value(param_val) {
  var internal_links = [];
  if (param_val.type == 'internal_link') {
    if (param_val.target) {
      internal_links.push(param_val.target);
    } else {
      Meguro.log("Null target in pull_internal_links_out_of_param_value (anchor_text="+param_val.anchor_text+")");
    }
  } else {
    for(var i=0; i < param_val.length; i++) {
      var sec = param_val[i];
      if (sec.type == 'internal_link') {
        if (sec.target) {
          internal_links.push(sec.target);
        } else {
          Meguro.log("Null target 2 in pull_internal_links_out_of_param_value (anchor_text="+sec.anchor_text+")");
        }
      }
    }
  }
  return internal_links;
}

function do_twitter(name, output) {
  if (name.match(/^\w+$/)) { // alphanumeric or _
    if (!output.web_sites) {
      output.web_sites = {};
    }
    do_top_level_field('http://twitter.com/'+name, output.web_sites, 'twitter');
    if (!output.nomenclature) {
      output.nomenclature = {};
    }
    do_top_level_field(name, output.nomenclature, 'twitter');
  }
}

function do_web_site(url, output, type) {
  if (!url.match(/^https?:\/\/[^\.]+\.[^\.]+/i)) {
    return;
  }
  if (!output.web_sites) {
    output.web_sites = {};
  }
  var parts = url.split('/');
  if (parts[1] != '') {
    Meguro.log("WHAT? Why not 2nd split part of URL blank? "+url);
    return;
  }
  url = parts[0].toLowerCase() + '//' + parts[2].toLowerCase();
  for (var i = 3; i < parts.length; ++i) {
    url += '/'+parts[i];
  }
  do_top_level_field(url, output.web_sites, type);
}

/*
 * Here we want to look through all the attributes of the Infobox and make
 * edges based on those
 */
function handle_template(key,template,output) {
  if (template.title) {
    if (template.title == 'twitter') {
      if (template.parameters && template.parameters["1"]) {
        do_twitter(template.parameters["1"], output);
      }
      return;
    }
    var template_key = keyify(template.title);
    var rules = ATTRIBUTE_RULES[template_key];
    var params = template.parameters;
    for (var param_name in params) {
      if (rules) {
        var rule = rules[param_name];
        if (rule) {
          var param_val = params[param_name];
          var internal_links = pull_internal_links_out_of_param_value(param_val);
          if (internal_links.length > 0) {
            var snippet = snippet_substitute(rule.template,key,internal_links[0]);
            emit_relation(key, internal_links[0], rule.category, rule.subcategory, snippet, rule.distance, rule.reverse_distance);
          }
        }
      }
      if (param_name.toLowerCase() == 'homepage' || param_name.toLowerCase() == 'website' || param_name.toLowerCase() == 'url') {
        var param_val = params[param_name];
        if (param_val) {
          if (typeof(param_val) == 'string' && param_val.match(/^http/)) {
            do_web_site(param_val, output, 'home_page');
          } else if (param_val.type && param_val.type == 'external_link' && param_val.target) {
            do_web_site(param_val.target, output, 'home_page');
          }
        }
      }
    }
  }
}

function emit_relation(key, target, category, subcategory, snippet, distance, reverse_distance) {
  if (target == key) { return; }
  var ed = new Object();
  ed.name = target.replace(/#.*/,'');
  ed.relationship_category = category;
  ed.relationship_subcategory = subcategory;
  ed.snippet = snippet;
  ed.distance = distance;
  ed.source_name = 'Wikipedia';
  ed.source_URL = 'http://en.wikipedia.org/wiki/'+encodeURIComponent(key.replace(/ /g, '_'));
  Meguro.emit(key,JSON.stringify({"relation":ed}));
  // Now make the reverse edge
  var reverse_key = ed.name;
  ed.name = key;
  ed.distance = reverse_distance;
  Meguro.emit(reverse_key,JSON.stringify({"relation":ed}));
}

function handle_category(json, output) {
  var cat = json.category;
  if (cat) {
    cat = cat.toLowerCase();
    if (cat.indexOf(' agencies') > 0 || cat.indexOf('living people') >= 0 || cat.indexOf('companies') >= 0 || cat.indexOf('organization') >= 0 || cat.indexOf('ministries') >= 0 || (cat.indexOf(' established') > 0 && cat.indexOf('states ') < 0 && cat.indexOf('places ') < 0 && cat.indexOf('settlements') < 0) || cat.indexOf('software') >= 0 || cat.indexOf(' music groups') >= 0 || cat.indexOf('schools') >= 0 || (cat.indexOf('universit') >= 0 && cat.indexOf('academics') < 0 && cat.indexOf('faculty') < 0 && cat.indexOf('alumni') < 0) || cat.indexOf('colleges') >= 0 || cat.indexOf('visitor attractions') >= 0 || cat.indexOf('museums') >= 0 || cat.indexOf('charities') >= 0) {
      if (!output.notes) {
        output.notes = {};
      }
      if (cat.indexOf('living people') >= 0) {
        output.notes.entity = 'person';
      } else {
        output.notes.entity = 'nonperson';
      }
    }
  }
}

function matches_acronym(str, acronym) {
  var p = str.split(/\s+/);
  var acronym_letters = acronym.split('');
  for (var i=0, il=p.length; i < il; ++i) {
    if (acronym_letters.length > p.length-i || !acronym_letters.length) {
      return false;
    }
    var letter = p[i].substr(0,1);
    if (letter.match(/[A-Z]/)) {
      if (letter != acronym_letters.shift()) {
        return false;
      }
    }
  }
  return (acronym_letters.length == 0);
}

/*
 * Iterate through the elemnts looking for links and if the anchor text or the
 * target matches the cleaned title, then we emit an alias for that target
 * pointing to the title.
 */
function handle_ambiguous_page(title,json) {
  var title_name = (title.charAt(0)=='(' ? title : title.replace(/\s*\(.+\)/,''));
  var acronym = false;
  if (title_name.match(/^[A-Z]+$/)) {
    acronym = true;
  }
  var fromLine = [];
  var bestLineDist = -1;
  for(var i=0, len = json.length; i < len; ++i) {
    var elem = json[i];
    if (typeof(elem) != 'string') {
      if (elem.type == 'heading') {
        if (elem.value && typeof(elem.value) == 'string') {
          var lv = elem.value.toLowerCase();
          if (lv == 'see also' || lv == 'similar spellings') {
            break;
          }
        }
      } else if (elem.type == 'internal_link' && elem.target) { // && elem.target.indexOf('#') < 0) 
        var distance = 90;
        var str = elem.anchor_text;
        if (!str) {
          str = elem.target.replace(/\s*\([^\(]+\)$/, '');
        }
        if (str == title_name) {
          distance = 10;
        } else if (acronym && matches_acronym(str, title_name)) {
          distance = 10;
        } else if (str.replace(/\s+((\S+\.)|(Entertainment)|(University)|(College)|(Company))$/,'') == title_name) {
          distance = 20;
        } else if (str.indexOf(title_name) >= 0) {
          distance = 70;
        }
        fromLine.push([elem.target, distance]);
        if (bestLineDist < 0 || distance < bestLineDist) {
          bestLineDist = distance;
        }
      }
    } else if (elem.indexOf("\n") >= 0) {
      for (var j=0, jl=fromLine.length; j < jl; ++j) {
        var distance = fromLine[j][1];
        if (distance < 30 || distance == bestLineDist) { // if there are multiple links on line, be pickier
          Meguro.emit(fromLine[j][0], JSON.stringify({'alias':title_name, 'fr':'d', 'distance':distance})); // fr:d means from disambiguation
        }
      }
      fromLine = [];
      bestLineDist = -1;
    }
  }
}

function index_to_bracketize(text, needle) {
  var needle_len = needle.length;
  var text_len = text.length;
  if (text_len < needle.length) {
    return -1;
  }
  if (text.substr(0, needle_len) == needle && (text_len == needle_len || text.charAt(needle_len).match(/[,\.'\s]/))) {
    return 0; //match at start
  }
  var ind = 0;
  while ((ind=text.indexOf(needle,ind)) >= 0) {
    if (text.charAt(ind-1) == ' ') { //space in front of word instead of just happening to be in middle of another word
      if (text_len == ind+needle_len || text.charAt(ind+needle_len).match(/[,\.'\s]/)) { //match at very end or match in middle with a space or something after
        return ind;
      }
    }
    ++ind;
  }
  return -1;
}

function bracketize_key(text, key, key_lower, key_alt) {
  var text_lower = text.toLowerCase();
  var len;
  var ind = index_to_bracketize(text_lower, key_lower);
  if (ind >= 0) {
    len = key_lower.length;
  } else {
    ind = index_to_bracketize(text_lower, key_alt);
    len = key_alt.length;
  }
  if (ind >= 0) {
    //Meguro.log('got '+key+' in '+text);
    return text.substr(0, ind) + '[[' + key + '|' + text.substr(ind, len) + ']]' + text.substr(ind+len);
  }
  //Meguro.log('did not get '+key+' in text '+text);
  return null;
}


function is_meta_page(title) {
  return (title.match(/^Wikipedia:/) || title.match(/^Portal:/) || title.match(/^File:/) || title.match(/^MediaWiki:/) || title.match(/^Category:/) || title.match(/^UN\/LOCODE:/) || title.match(/^Book:/) || title.match(/^Help:/) || title.match(/^P:/) || title.match(/^List of /));
}

function map(key,value) {
  if (is_meta_page(key)) { return; }
  var json;
  try {
    json = JSON.parse(value);
  } catch(e) {
    Meguro.log("JSON.parse problem on key="+key+". value.length="+value.length);
    return;
  }
  if (!json) { return; }
  if (page_is_ambiguous(key,json)) {
    handle_ambiguous_page(key,json);
    return;
  }
  var output = {};
  var outlinks_a = [];
  var outlinks_h = {};
  var paragraph_so_far = '';
  var targets_in_sentence = [];
  var prev_sentence_end = -1;
  var key_lower = key.toLowerCase();
  var key_alt = convert_to_query(key);
  var have_bracketed_key = false;
  var bracketed;
  for(var i=0, il=json.length; i < il; ++i) {
    if (typeof(json[i]) == 'string') {
      var str = json[i];
      // first see if a sentence ender in here and do targets in that sentence (only if not too long already)
      if (targets_in_sentence && targets_in_sentence.length > 0) {
        if (!paragraph_so_far.match(/^[:\*\|#]/)) {
          var sentence_ending_arr = str.match(/^[^\n]*?\."?\)?/);
          if (sentence_ending_arr) {
            if (!have_bracketed_key && (bracketed=bracketize_key(sentence_ending_arr[0], key, key_lower, key_alt))) {
              have_bracketed_key = true;
              sentence_ending_arr[0] = bracketed;
            } 
            var snippet = paragraph_so_far + sentence_ending_arr[0];
            var snippet_length = snippet.length;
            var commas_in_sentence = snippet.substr(prev_sentence_end+1).match(/[,;]/g);
            if (!commas_in_sentence || commas_in_sentence.length <= 4 || !output.descriptions) { // not weird run-on sentence, unless first paragraph in which case assume fine
              for (var j=0, jl=targets_in_sentence.length; j < jl; ++j) {
                var ti = targets_in_sentence[j];
                var clean_snippet = snippet.substr(0,ti.start_index) + '[[' + ti.key + '|' + ti.text + ']]' + snippet.substr(ti.start_index+ti.text.length);
                //Meguro.log(ti.key+': '+clean_snippet);
                emit_relation(key, ti.key, 'UNCLASSIFIED', 'UNCLASSIFIED', clean_snippet, -1, -1);
              }
              targets_in_sentence = [];
              prev_sentence_end = snippet.lastIndexOf('.');
            }
            else { // give up on this paragraph
              targets_in_sentence = null;
            }
          }
        } else {
          targets_in_sentence = null;
        }
      }
      var ind = str.indexOf("\n");
      if (ind >= 0) { //finished paragraph
        if (ind > 0 || paragraph_so_far.length > 0) { // don't want to take just a single stray newline before we've got real text
          var addition = str.substr(0, ind);
          if (!have_bracketed_key && (bracketed=bracketize_key(addition, key, key_lower, key_alt))) {
            have_bracketed_key = true;
            addition = bracketed;
          } 
          paragraph_so_far += addition;
          if (!output.descriptions) { // means first paragraph
            output.descriptions = [];
            output.descriptions[0] = {};
            do_top_level_field(paragraph_so_far.replace(/\s?\([^\(\)]*\)/g,''), output.descriptions[0], 'text');
            do_top_level_field(source_name, output.descriptions[0], 'source_name');
            do_top_level_field('http://en.wikipedia.org/wiki/'+encodeURIComponent(key.replace(/ /g, '_')), output.descriptions[0], 'source_URL');
          }
        }
        targets_in_sentence = [];
        ind = str.lastIndexOf("\n");
        addition = str.substr(ind+1).replace(/^[\n\s]+/,'');
        have_bracketed_key = false;
        if (bracketed=bracketize_key(addition, key, key_lower, key_alt)) {
          have_bracketed_key = true;
          addition = bracketed;
        } 
        paragraph_so_far = addition;
        prev_sentence_end = paragraph_so_far.lastIndexOf('.');
      } else {
        if (!have_bracketed_key && (bracketed=bracketize_key(str, key, key_lower, key_alt))) {
          have_bracketed_key = true;
          str = bracketed;
        } 
        paragraph_so_far += str;
      }
    } else {
      switch (json[i].type) {
        case 'template':
          handle_template(key,json[i], output);
          break;
        case 'redirect':
          if (handle_redirect(key,json[i], output)) {
            return;
          }
          break;
        case 'category':
          if (!output.notes || !output.notes.entity) {
            handle_category(json[i], output);
          }
          break;
        case 'internal_link':
          var link = json[i];
          var link_target = link.target;
          if (link_target) {
            link_target = link_target.replace(/#.*/, '');
            var good_target = (!is_meta_page(link_target));
            if (link.anchor_text) {
              if (targets_in_sentence && good_target) {
                targets_in_sentence.push({key:link_target, start_index:paragraph_so_far.length, text:link.anchor_text});
              }
              paragraph_so_far += link.anchor_text;
            } else {
              if (targets_in_sentence && good_target) {
                targets_in_sentence.push({key:link_target, start_index:paragraph_so_far.length, text:link_target});
              }
              paragraph_so_far += link_target;
            }
            if (!outlinks_h.hasOwnProperty(link_target) && good_target) {
              outlinks_h[link_target] = 1;
              outlinks_a.push(link_target);
            }
          } else if (link.anchor_text) {
            paragraph_so_far += link.anchor_text;
          }
          break;
        case 'external_link':
          var link = json[i];
          var target = link.target;
          if (target && link.anchor_sections && link.anchor_sections.length > 0 && typeof(link.anchor_sections[0]) == 'string') {
            var label = link.anchor_sections[0].toLowerCase();
            if ((!output.web_sites || !output.web_sites.home_page) && (label.indexOf('official website') >= 0 || label.indexOf('official web site') >= 0 || label.indexOf('homepage') >= 0 || label.indexOf('home page') >= 0 || label == key.replace(/\s\(.*\)$/,'').toLowerCase())) {
              do_web_site(target, output, 'home_page');
            } else if ((!output.web_sites || !output.web_sites.twitter) && target.match(/^http:\/\/twitter.com\/\S+$/) && (label.indexOf('official twitter') == 0)) {
              do_twitter(target.substr(target.lastIndexOf('/')+1), output);
            }
          }
          break;
      }
    }
  }
  if (output.descriptions) { //this isn't just a garbage page
    do_top_level_field((key.charAt(0)=='(' ? key : key.replace(/\s*\([^\(]+\)$/,'')), output, 'common_name');
    if (!output.nomenclature) {
      output.nomenclature = {};
    }
    output.nomenclature.wikipedia = key;
    Meguro.emit(key,JSON.stringify({about:output}));
    Meguro.emit(key,JSON.stringify({ol:outlinks_a}));
  }
}




function reduce(key, values) {
  /* used to output dictionary of redirecter to redirect target */
  /*
  for (var i=0, il=values.length; i < il; ++i) {
    var v = values[i];
    if (v == '') {
      continue;
    }
    v = JSON.parse(v);
    if (v.alias && v.fr == 'r') {
      Meguro.save(v.alias, key);
    }
  }
  */
  // now just save out a concatenation of the emits in an array
  Meguro.save(key, JSON.stringify(values));
}

function convert_to_query(word) {
  /* first, see if this might be a name (by seeing if 1-3 words in the "word" that are all capitalized... */
  var parts = word.split(/\s+/);
  var nparts = parts.length;
  if (nparts > 1 && nparts < 5) {
    var noncapital = 0;
    for (var i = 0; i < nparts; ++i) {
      if (!parts[i].match(/^[A-Z]/)) {
        noncapital = 1;
        break;
      }
    }
    // if so, just look at last part
    if (!noncapital && parts[nparts-1].length > 2) {
      if (word.match(/\.$/)) {
        word = parts[nparts-2].replace(/[,:;!]$/,'');
      }
      else if (parts.length > 3) {
        word = parts[0]+' '+parts[1];
      }
      else if (parts[nparts-1] == 'University' || parts[nparts-1] == 'College') {
        parts.pop();
        word = parts.join(' ');
      }
      else {
        word = parts[nparts-1];
      }
    }
  }
  return word.toLowerCase();
}

