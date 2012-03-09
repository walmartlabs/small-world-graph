class WikiCrawler
  IMAGE_PAGE = 6
  CATEGORY_PAGE = 14
  IMAGE_EXTENSIONS = ["jpg","gif","png","jpeg"]

  def initialize(conn,builder,verbose = false)
    @page_ids = {}
    @cat_stack = []
    @page_search = conn.prepare("SELECT page_namespace,page_title from page where page_id = ?")
    @category_search = conn.prepare('SELECT cl_from,cl_to,cl_sortkey,cl_timestamp from categorylinks where cl_to = ?')
    @builder = builder
    @verbose = verbose
  end

  def drill_page(page_id)
    res = @page_search.execute(page_id)
    row = res.fetch
    if row
      page_namespace = row[0].to_i
      page_title = row[1]
      res.free_result();
      case page_namespace
      when IMAGE_PAGE
        if @page_ids[page_id].nil? and page_title =~ /\.(jpg)|(png)|(gif)|(jpeg)$/
          puts "Found image page #{page_id} : #{page_title}" if @verbose
          begin
            image = Image.new(page_id,page_title)
            image.to_xml_snippet
          rescue
            puts $!
          end
        @page_ids[page_id] = true
        end
      when CATEGORY_PAGE
        puts "Found category page #{page_id} : #{page_title}" if @verbose
        drill_category(page_title) do |id|
          if @cat_stack.include?(id)
            puts "Aborting out of page loop"
            return
          end
          @cat_stack.push(id)
          drill_page(id)
          @cat_stack.pop
        end
      else
        #puts "Unknown Namespace for #{page_title}"
      end
    else
      #puts "Unknown Page ID: #{page_id}"
    end
  end

  def drill_category(category_name,&block)
    res = @category_search.execute(category_name)
    puts "Drilling into #{category_name} : Found #{res.num_rows}" if @verbose
    ids_to_check = []
    res.each do |row|
      page_id = row[0].to_i
      ids_to_check << page_id
    end
    res.free_result();
    if block
      ids_to_check.each do |page_id|
        yield page_id
      end
    else
      return ids_to_check
    end
  end
end
