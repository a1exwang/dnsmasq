#include "dnsmasq.h"

void dump_cache2 ();

#define MAX_DOMAINS 1000
int ndomain = 0;
static char *domains[MAX_DOMAINS];

void
route_init ()
{
  FILE *fp = fopen ("gfwlist.txt", "r");

  if (fp == NULL)
    {
      perror ("Failed: ");
      exit (1);
    }

  char buffer[1024];
  printf ("Loading gfwlist.txt\n");
  while (fgets (buffer, sizeof (buffer), fp) && ndomain < MAX_DOMAINS)
    {
      buffer[strcspn (buffer, "\n")] = 0;
      char *s = malloc (strlen (buffer));
      // just let it leak
      strcpy (s, buffer);
      printf ("  gfw domain: %s\n", s);
      domains[ndomain++] = s;
    }

  fclose (fp);
}

static int
endswith (const char *s, const char *target)
{
  int n = strlen (s);
  int ntarget = strlen (target);
  if (n < ntarget)
    {
      return 0;
    }
  for (int i = 0; i < ntarget; i++)
    {
      if (s[n - i - 1] != target[ntarget - i - 1])
        {
          return 0;
        }
    }
  return 1;
}

int
is_route_domain (const char *name)
{
  for (int i = 0; i < ndomain; i++)
    {
      if (endswith (name, domains[i]))
        {
          return 1;
        }
    }
  return 0;
}

int
add_route (const char *target)
{

  static char route_cmd[256];
  sprintf (route_cmd, "sudo ip r add %s dev wg1 table 10", target);
  printf ("command = %s\n", route_cmd);
  int ret = system (route_cmd);
  return ret;
}

void
handle_name_reply (int flags, const char *name, union all_addr *addr)
{
  if (!(flags & F_KEYTAG) && !(flags & F_RCODE) && (flags & F_IPV4))
    {
      inet_ntop (flags & F_IPV4 ? AF_INET : AF_INET6, addr, daemon->addrbuff,
                 ADDRSTRLEN);
      if (is_route_domain (name))
        {
          struct timeval tv;
          gettimeofday (&tv, NULL);
          unsigned long long us0
              = (unsigned long long)(tv.tv_sec) * 1000 * 1000
                + (unsigned long long)(tv.tv_usec);

          int r = add_route (daemon->addrbuff);

          gettimeofday (&tv, NULL);
          unsigned long long us1
              = (unsigned long long)(tv.tv_sec) * 1000 * 1000
                + (unsigned long long)(tv.tv_usec);

          printf ("DNS_route_reply(%d): %s %s, time = %f\n", r, name,
                  daemon->addrbuff, (us1 - us0) / 1000.0);
        }
      else
        {
          printf ("DNS_reply: %s %s\n", name, daemon->addrbuff);
        }
    }
}

void
route_cache_dump ()
{
  dump_cache2 ();
}