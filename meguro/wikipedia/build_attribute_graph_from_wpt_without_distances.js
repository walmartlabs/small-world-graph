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
      "distance": 20
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
      "distance": 10
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
    Meguro.emit(redirect.target,JSON.stringify({'alias':key}));
  }
}

/*
 * Pull all of the internal links out of the param value of an infobox
 */
function pull_internal_links_out_of_param_value(param_val) {
  var internal_links = [];
  if (param_val.type == 'internal_link') {
    internal_links.push(param_val.target);
  } else {
    for(var i=0; i < param_val.length; i++) {
      var sec = param_val[i];
      if (sec.type == 'internal_link') {
        internal_links.push(sec.target);
      }
    }
  }
  return internal_links;
}

/*
 * Here we want to look through all the attributes of the Infobox and make
 * edges based on those
 */
function handle_template(key,template) {
  if (template.title) {
    var template_key = keyify(template.title);
    var rules = ATTRIBUTE_RULES[template_key];
    if (rules) {
      var params = template.parameters;
      for (var param_name in params) {
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
    }
  }
}

function emit_relation(key, target, category, subcategory, snippet, distance, reverse_distance) {
  var ed = new Object();
  ed.name = target;
  ed.relationship_category = category;
  ed.relationship_subcategory = subcategory;
  ed.snippet = snippet;
  ed.distance = distance;
  ed.source_name = 'Wikipedia';
  ed.source_URL = 'http://en.wikipedia.org/wiki/'+encodeURIComponent(key);
  Meguro.emit(key,JSON.stringify({"relation":ed}));
  // Now make the reverse edge
  var reverse_key = ed.name;
  ed.name = key;
  ed.distance = reverse_distance;
  Meguro.emit(reverse_key,JSON.stringify({"relation":ed}));
}

/*
 * Iterate through the elemnts looking for links and if the anchor text or the
 * target matches the cleaned title, then we emit an alias for that target
 * pointing to the title.
 */
function handle_ambiguous_page(title,json) {
  var title_name = (title.charAt(0)=='(' ? title : title.replace(/\s*\(.+\)/,''));
  for(var i=0, len = json.length; i < len; ++i) {
    var elem = json[i];
    if (typeof(elem) != 'string' && elem.type == 'internal_link') {
      if ((elem.anchor_text && elem.anchor_text.indexOf(title) >= 0) || elem.target.indexOf(title) >= 0) {
        Meguro.emit(elem.target,JSON.stringify({'alias':title_name}));
      }     
    }
  }
}

function map(key,value) {
  if (key.match(/^Wikipedia:/)) { return; }
  var json = JSON.parse(value);
  if (!json) { return; }
  if (page_is_ambiguous(key,json)) {
    handle_ambiguous_page(key,json);
    return;
  }
  var output = {};
  var outlinks = [];
  var paragraph_so_far = '';
  var targets_in_sentence = [];
  var prev_sentence_end = -1;
  for(var i=0, il=json.length; i < il; ++i) {
    if (typeof(json[i]) == 'string') {
      var str = json[i];
      // first see if a sentence ender in here and do targets in that sentence (only if not too long already)
      if (targets_in_sentence && targets_in_sentence.length > 0) {
        if (paragraph_so_far.charAt(0) != ':' && paragraph_so_far.charAt(0) != '*') {
          var sentence_ending_arr = str.match(/^[^\n]*?[a-z\]][a-z\]]\./);
          if (sentence_ending_arr) {
            var snippet = paragraph_so_far + sentence_ending_arr[0];
            var commas_in_sentence = snippet.substr(prev_sentence_end+1).match(/[,;]/g);
            if (!commas_in_sentence || commas_in_sentence.length <= 4) { // not weird run-on sentence
              for (var j=0, jl=targets_in_sentence.length; j < jl; ++j) {
                emit_relation(key, targets_in_sentence[j], 'UNCLASSIFIED', 'UNCLASSIFIED', snippet, -1, -1);
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
          paragraph_so_far += str.substr(0, ind);
          if (!output.descriptions) { // means first paragraph
            output.descriptions = [];
            output.descriptions[0] = {};
            do_top_level_field(paragraph_so_far.replace(/\s?\([^\(\)]*\)/g,''), output.descriptions[0], 'text');
            do_top_level_field(source_name, output.descriptions[0], 'source_name');
            do_top_level_field('http://en.wikipedia.org/wiki/'+encodeURIComponent(key), output.descriptions[0], 'source_URL');
          }
          targets_in_sentence = [];
          var nline = str.lastIndexOf("\n");
          if (nline < 0) {
            nline = ind;
          }
          paragraph_so_far = str.substr(nline+1).replace(/^[\n\s]+/,'');
          prev_sentence_end = paragraph_so_far.lastIndexOf('.');
        } 
      } else {
        paragraph_so_far += str;
      }
    } else {
      switch (json[i].type) {
        case 'template':
          handle_template(key,json[i]);
          break;
        case 'redirect':
          handle_redirect(key,json[i], output);
          break;
        case 'internal_link':
          var link = json[i];
          if (targets_in_sentence) {
            targets_in_sentence.push(link.target);
          }
          if (link.anchor_text) {
            paragraph_so_far += '[['+link.target+'|'+link.anchor_text+']]';
          } else {
            paragraph_so_far += '[['+link.target+']]';
          }
          var saw_this_outlink = 0;
          for (var j=0, jl=outlinks.length; j < jl; ++j) {
            if (outlinks[j] == link.target) {
              saw_this_outlink = 1;
              break;
            }
          }
          if (!saw_this_outlink) {
            outlinks.push(link.target);
            //X Meguro.emit(link.target, JSON.stringify({inl:key}));
          }
          break;
      }
    }
  }
  if (output.descriptions) { //this isn't just a redirect
    do_top_level_field((key.charAt(0)=='(' ? key : key.replace(/\s*\(.+\)/,'')), output, 'common_name');
    Meguro.emit(key,JSON.stringify({about:output})); //Meguro.emit(key,JSON.stringify({about:output}, {outl:outlinks}));
  }
}


// ---------------------------------------------
// -------- R e d u c e r ----------------------
// ---------------------------------------------


/*
 * Make a final graph entry based on the different emitted values
 */
function reduce(key,values) {
  var is_not_just_alias = false;
  var output = {};

  // first go through array and figure out a hash of common inlink counts, also checking here that this is not an alias entry
  var outlinks_of_inlinks_counts = {};
  var inlinks_count = 1; // because count self as linking to self
  for (var i = 0, il=values.length; i < il; ++i) {
    if (values[i] == '') { continue; }
    var v = JSON.parse(values[i]);
    if (v.about) {
      v = v.about;
      for (var k in v) {
        output[k] = v[k];
      }
      is_not_just_alias = true;
    } else if (v.inl) {
      inlinks_count += 1;
      var dv = Meguro.dictionary(v.inl);
      if (!dv) { Meguro.log("no outlinks for "+v.inl); exit; }
      var outlinks_of_link = JSON.parse(dv); 
      for (var j = 0, jl=outlinks_of_link.length; j < jl; ++j) {
        var n = outlinks_of_link[j]; //a page linked to from a page that also links to key
        if (n != key) {
          outlinks_of_inlinks_counts[n] = (outlinks_of_inlinks_counts.hasOwnProperty(n) ? outlinks_of_inlinks_counts[n]+1 : 1);
        }
      }
    }
  }
  if (!is_not_just_alias) {
    return;
  }
  // also look at things this page itself links to
  /*
  var outlinks = Meguro.dictionary(key);
  if (!outlinks) {
    Meguro.log("No outlinks for "+key);
    exit;
  }
  outlinks = JSON.parse(outlinks);
  for (var i = 0, il=outlinks.length; i < il; ++i) {
    var n = outlinks[i];
    outlinks_of_inlinks_counts[n] = (outlinks_of_inlinks_counts.hasOwnProperty(n) ? outlinks_of_inlinks_counts[n]+1 : 1);
  }
  */

  // now go through again and actually consider "about" information, edges, etc.
  var output_edges = {};
ITER:
  for (var i = 0, il=values.length; i < il; ++i) {
    if (values[i] == '') { continue; }
    var v = JSON.parse(values[i]);
    if (v.alias) {
      if (!output.aliases) {
        output.aliases = [v.alias];
      } else {
        if (output.aliases.indexOf(v.alias) < 0) {
          output.aliases.push(v.alias);
        }
      }
    } else if (v.relation) {
      v = v.relation;
      /*
      if (v.distance < 0) {
        v.distance = Math.round(245*(1-outlinks_of_inlinks_counts[v.name]/inlinks_count)) + 10;
      }
      */
      var edge = null;
      var desc_ind = null;
      if (output_edges.hasOwnProperty(v.name)) {
        edge = output_edges[v.name];
        for (var j=0, jl=edge.descriptors.length; j < jl; ++j) {
          var d = edge.descriptors[j];
          if (d.relationship_category == v.relationship_category && d.relationship_subcategory == v.relationship_subcategory) {
            if (d.relationship_category == 'UNCLASSIFIED') { // in-text
              edge.descriptors[j].snippet = choose_snippet(d.snippet, v.snippet, v.name, key);
              continue ITER;
            }
            if (d.distance <= v.distance) {
              continue ITER; //already have descriptor worth keeping about this
            }
            desc_ind = j;
          }
          break;
        }
        if (desc_ind == null) { // no matching descriptor
          desc_ind = edge.descriptors.length;
          edge.descriptors.push({});
        }
        if (edge.distance > v.distance) {
          edge.distance = v.distance;
        }
      } else { // new edge
        edge = {"descriptors":[{}], "distance":v.distance};
        output_edges[v.name] = edge;
        desc_ind = 0;
      }
      for (var k in v) { 
        if (k != 'name') {
          edge.descriptors[desc_ind][k] = v[k];
        }
      }
    } else if (!v.inl && !v.about) {
      Meguro.log("Why unknown in reduce: "+v);
      exit;
    }
  }

  output.edges = output_edges;
  var now = new Date();
  output.last_updates = {};
  output.last_updates[source_name] = now.toGMTString();
  Meguro.save(key, JSON.stringify(output));
}




/* Given multiple snippets, choose the best one according to the shorter one that (ideally) mentions both concepts. */
function choose_snippet(snippet1, snippet2, key1, key2) {
  var key_a = convert_to_query(key1);
  var pat1 = new RegExp(key_a, 'i');
  key_a = convert_to_query(key2);
  var pat2 = new RegExp(key_a, 'i');
  var snippets = undefined;
  if (snippet1.length < snippet2.length) {
    snippets = [snippet1, snippet2];
  } else {
    snippets = [snippet2, snippet1];
  }
  /* first see if any of snippets contains both names */
  for (var i = 0, il=snippets.length; i < il; ++i) {
    var s = snippets[i];
    if ((rel_match(s, key1) || s.match(pat1)) && (rel_match(s, key2) || s.match(pat2))) {
      return snippets[i];
    }
  }
  return snippets[0];
}


function rel_match(where, what) {
  return (where.indexOf("[["+what+"|")>=0 || where.indexOf("[["+what+"]")>=0);
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

