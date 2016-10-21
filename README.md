# nginx-plugin

一般情况下，当用户安装云锁的时候，云锁会自动适配nginx版本，使用我们已经预编译好的包含云锁模块的nginx备份并替换掉您当前系统中使用的nginx。卸载时，会将系统原始nginx文件替换回来。因此，云锁可保护使用nginx搭建的网站，开创了这个领域的先河。
我们秉承着让安全变得更简单的宗旨，使云锁尽可能多的适配广大用户的nginx版本（目前无法覆盖所有nginx版本），决定开放云锁nginx模块的源码，让有能力的小伙伴们自己动手同云锁一起打造一个安全的nginx环境。

> 此文档假设您曾经编译过nginx或tengine源代码，如果您之前没有经验，请参考：http://nginx.org/en/docs/configure.html
	
## 1. 如何编译云锁nginx模块

步骤如下：
- 1.为避免意外情况发生， 请先将系统当前使用中的nginx进行备份(包括相关的网站配置文件)
- 2.wget https://codeload.github.com/yunsuo-open/nginx-plugin/zip/master -O nginx-plugin-master.zip
- 3.unzip nginx-plugin-master.zip
- 4.cd nginx-plugin-master
- 5.pwd 获取当前云锁插件源码所在目录的全路径 (假设为：/home/nginx-plugin-master，实际情况以pwd输出为准)	
- 6.以下两种情况， 可以跳过这一步骤：
	1）您的 nginx 是 tengine。 
	2）nginx 版本大于等于 1.8.0 并且 安装的云锁为V3， 此时需要关注 第 8 条说明。
	
	除以上两种情况，对于 nginx 来说，由于其不支持post过滤，所以需要修改nginx源码目录下src/http/ngx_http_upstream.c 文件,步骤如下：

    - a.查找 static void ngx_http_upstream_init_request(ngx_http_request_t *r)函数，在其所在行上方添加：int ngx_http_yunsuo_post_in_handler(ngx_http_request_t *r);
    - b.在ngx_http_upstream_init_request函数开头，变量声明后，添加：

            if(ngx_http_yunsuo_post_in_handler(r)) 
            {
                return;
            }

    > 什么？没看懂？好吧，以nginx-1.0.11为例:
 
    - 修改前源码：
 
            static void
            ngx_http_upstream_init_request(ngx_http_request_t *r)
            {
                ngx_str_t                      *host;
                ngx_uint_t                      i;
                ngx_resolver_ctx_t             *ctx, temp;
                ngx_http_cleanup_t             *cln;
                ngx_http_upstream_t            *u;
                ngx_http_core_loc_conf_t       *clcf;
                ngx_http_upstream_srv_conf_t   *uscf, **uscfp;
                ngx_http_upstream_main_conf_t  *umcf;

                if (r->aio) {
                    return;
                }

                u = r->upstream;
                
                ......
            }
    
    - 修改后源码：
 
            /*这段是添加的*/
            int ngx_http_yunsuo_post_in_handler(ngx_http_request_t *r);
            /*------------*/

            static void
            ngx_http_upstream_init_request(ngx_http_request_t *r)
            {
                ngx_str_t                      *host;
                ngx_uint_t                      i;
                ngx_resolver_ctx_t             *ctx, temp;
                ngx_http_cleanup_t             *cln;
                ngx_http_upstream_t            *u;
                ngx_http_core_loc_conf_t       *clcf;
                ngx_http_upstream_srv_conf_t   *uscf, **uscfp;
                ngx_http_upstream_main_conf_t  *umcf;

                /*这段是添加的*/
                if(ngx_http_yunsuo_post_in_handler(r)) 
                {
                    return;
                }
                /*------------*/
                
                if (r->aio) {
                    return;
                }

                u = r->upstream;
                
                ......
                
            }


- 7.云锁的nginx插件模块是标准的nginx模块，所以您在编译nginx过程中，configure时只要添加额外参数--add-module=/home/nginx-plugin-master(注意：/home/nginx-plugin-master为示例，实际路径以步骤5中pwd命令为准）即可让nginx支持云锁的功能,示例如下：

    - 假设您之前configure时的命令如下：

            ./configure --prefix=/usr/local/nginx --with-http_stub_status_module \
                --with-http_ssl_module --with-http_gzip_static_module \
                --add-module=../ngx_cache_purge-1.3 
    
    - 现在的configure时的命令如下：

            ./configure --prefix=/usr/local/nginx --with-http_stub_status_module \
                --with-http_ssl_module --with-http_gzip_static_module \
                --add-module=../ngx_cache_purge-1.3  --add-module=/home/nginx-plugin-master

- 8.当您的 nginx 版本大于等于 1.8.0 并且安装的云锁为V3时，想要支持 POST 防护，
只需在 configure 生成的 Makefile （即 objs/Makefile 文件）中 CFLAGS 追加宏定义 HIGHERTHAN8
	形如 ：	CFLAGS =  -pipe  -O -W -Wall -Wpointer-arith -Wno-unused-parameter -Werror -g  
	修改为：CFLAGS =  -pipe  -O -W -Wall -Wpointer-arith -Wno-unused-parameter -Werror -g -DHIGHERTHAN8
	
- 9.编译 nginx (注意：如果原本已经有 nginx, 只执行 make 即可，make install 会覆盖掉你的 nginx.conf)
- 10.将系统当前使用中的nginx二进制文件替换为刚刚编译好的包含了云锁模块的nginx文件即可
	

## 2. 让云锁识别您自己编译的nginx
1. 安装云锁，如果您已经安装了云锁，可跳过此步骤。如果还没有，请到http://www.yunsuo.com.cn/ht/software/下载并安装云锁
2. cd /usr/local/yunsuo_agent/nginx/
3. ./configure_compile_nginx nginx_install_path (nginx_install_path为nginx的安装路径,即configure时 --prefix=path如果未指定过路径, 那么默认为/usr/local/nginx)
	   

## FAQ：
### Q1.什么情况下我需要自己编译云锁的nginx模块？

- a.当您的nginx使用了第三方或者自己开发的模块的时候，需要编译云锁的nginx模块。您可以通过nginx -V命令查看输出的信息里是否包含了 --add-module= 的字样 (例如：--add-module=../ngx_cache_purge-1.3 说明使用了ngx_cache_purge-1.3第三方模块)
- b.当使用tengine的时候，需要编译云锁的nginx模块
- c.当您发现当前使用的nginx版本比我们自动安装的版本高的时候，可以自己编译云锁的nginx模块

### Q2.如果我把云锁卸载了，nginx需要重新编译吗？

- 不需要，云锁的nginx模块会判断云锁是否安装，如果不安装则不生效。当然您也可以替换回之前的nginx

### Q3.我应该先安装云锁，还是先编译nginx?
- 都可以，没有先后顺序关系
    
### Q4.怎样单独卸载 nginx 插件？
有如下三种方式可以实现卸载插件：
    
- 方式一：现有版本 nginx 默认不支持从客户端卸载，如果想支持从客户端卸载， 需要手动将 系统原有的 nginx 重命名为 nginx.bak, 并将之替换 /usr/local/yunsuo_agent/nginx/backup 目录下的 nginx.bak（此操作需要关闭云锁自保护功能）， 这样就可以使用客户端的插件卸载功能了
- 方式二：手动删除或者重命名 /usr/local/yunsuo_agent/nginx/ 目录下的 libnginx_plugin.so（此操作需要关闭云锁自保护功能）， 重启 nginx 服务即可
- 方式三：手动使用系统原有的 nginx 直接替换 当前使用的带有云锁插件的 nginx
        
> 推荐使用第一种方式， 因为其便于后续的安装和卸载
		
### Q5.编译时可能出现的几种错误解决方法
	1、 遇如下错误信息 ： cc1: all warnings being treated as errors， 编译器把警告信息作为错误处理了
		解决： 修改 objs/Makefile 
		把 CFLAGS =  -pipe  -O -W -Wall -Wpointer-arith -Wno-unused-parameter -Werror -g
		修改为：CFLAGS =  -pipe  -O -W -Wall -Wpointer-arith -Wno-unused-parameter  -g 即 去掉 -Werror 选项
		重新 make， 注意是重新 make 不要重新 ./configure 
	
	2、 遇如下错误信息： undefined reference to `dlclose'， 需要在链接时， 加入-ldl 选项
		解决： 修改 objs/Makefile
		搜索 -lpthread， 定位到该行结束， 加入 -ldl
		形如 -lpthread -lcrypt 修改为 -lpthread -lcrypt -ldl
		重新 make， 注意是重新make 不要重新 ./configure 
		
www.yunsuo.com.cn

