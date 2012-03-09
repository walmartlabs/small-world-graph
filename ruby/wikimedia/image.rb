class Image
  attr_reader :name,:width,:height,:link,:description,:metadata,:categories

  def initialize(page_id,name)
    res = @@image_search.execute(name)
    raise "Could not find #{name}" if res.num_rows == 0
    row = res.fetch
    @name = name
    @width = row[0].to_i
    @height = row[1].to_i
    @description = row[2]
    @metadata = row[3]
    md5sum = Digest::MD5.hexdigest(name)
    @link = 'http://upload.wikimedia.org/wikipedia/commons/' + md5sum[0,1] + "/" + md5sum[0,2] + "/" + name
    res.free_result
    res = @@img_cat_search.execute(page_id)
    @categories = []
    res.each do |row|
      @categories << row[0]
    end
  end

  def to_xml_snippet
    @@builder.image(:name => name, :width => width, :height => height, :link => link) do |i|
      i.description self.description
      i.metadata self.metadata
      categories.each do |cat|
        i.category cat
      end
    end
  end
  
  def self.initialize(conn,builder)
    @@image_search = conn.prepare('SELECT img_width,img_height,img_description,img_metadata FROM image where img_name = ?')
    @@img_cat_search = conn.prepare('SELECT cl_to from categorylinks where cl_from = ?')
    @@builder = builder
  end

end
