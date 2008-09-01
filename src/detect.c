/* Basic detection engine datastructure */

#include <pcre.h>

#include "vips.h"
#include "debug.h"
#include "detect.h"
#include "flow.h"

#include "detect-parse.h"
#include "detect-siggroup.h"

#include "detect-address.h"
#include "detect-content.h"
#include "detect-uricontent.h"
#include "detect-pcre.h"
#include "detect-depth.h"
#include "detect-nocase.h"
#include "detect-recursive.h"
#include "detect-rawbytes.h"
#include "detect-within.h"
#include "detect-distance.h"
#include "detect-offset.h"
#include "detect-sid.h"
#include "detect-priority.h"
#include "detect-classtype.h"
#include "detect-reference.h"
#include "detect-threshold.h"
#include "detect-metadata.h"
#include "detect-msg.h"
#include "detect-rev.h"
#include "detect-flow.h"
#include "detect-dsize.h"
#include "detect-flowvar.h"
#include "detect-noalert.h"

#include "action-globals.h"
#include "detect-mpm.h"
#include "tm-modules.h"

#include "util-unittest.h"

/* XXX store all this in a DetectionEngineCtx one day */
static Signature *sig_list = NULL;
static DetectAddressGroupsHead *g_src_gh = NULL, *g_tmp_gh = NULL;

SigMatch *SigMatchAlloc(void);
void SigMatchFree(SigMatch *sm);

/* tm module api functions */
int Detect(ThreadVars *, Packet *, void *);
int DetectThreadInit(ThreadVars *, void **);
int DetectThreadDeinit(ThreadVars *, void *);

void TmModuleDetectRegister (void) {
    tmm_modules[TMM_DETECT].name = "Detect";
    tmm_modules[TMM_DETECT].Init = DetectThreadInit;
    tmm_modules[TMM_DETECT].Func = Detect;
    tmm_modules[TMM_DETECT].Deinit = DetectThreadDeinit;
    tmm_modules[TMM_DETECT].RegisterTests = NULL;
}

void SigLoadSignatures (void)
{
    Signature *prevsig = NULL, *sig;

    /* The next 3 rules handle HTTP header capture. */

    /* http_uri -- for uricontent */
    sig = SigInit("alert tcp any any -> any any (msg:\"HTTP URI cap\"; flow:to_server; content:\"GET \"; depth:4; pcre:\"/^GET (?P<http_uri>.*) HTTP\\/\\d\\.\\d\\r\\n/G\"; depth:400; noalert; sid:1;)");
    if (sig) {
        prevsig = sig;
        sig_list = sig;
    }

    /* http_host -- for the log-httplog module */
    sig = SigInit("alert tcp any any -> any any (msg:\"HTTP host cap\"; flow:to_server; content:\"Host:\"; depth:400; pcre:\"/^Host: (?P<http_host>.*)\\r\\n/m\"; noalert; sid:2;)");
    if (sig == NULL)
        return;
    prevsig->next = sig;
    prevsig = sig;

    /* http_ua -- for the log-httplog module */
    sig = SigInit("alert tcp any any -> any any (msg:\"HTTP UA cap\"; flow:to_server; content:\"User-Agent:\"; depth:400; pcre:\"/^User-Agent: (?P<http_ua>.*)\\r\\n/m\"; noalert; sid:3;)");
    if (sig == NULL)
        return;
    prevsig->next = sig;
    prevsig = sig;

/*
    sig = SigInit("alert tcp any any -> any any (msg:\"ViCtOr nocase test\"; sid:4; rev:13; content:ViCtOr; nocase; depth:150;)");
    if (sig == NULL)
        return;
    prevsig->next = sig;
    prevsig = sig;

    sig = SigInit("alert ip any any -> 1.2.3.4 any (msg:\"ViCtOr case test\"; sid:2001; content:ViCtOr; depth:150;)");
    if (sig == NULL)
        return;
    prevsig->next = sig;
    prevsig = sig;

    sig = SigInit("alert ip ANY any -> 192.168.0.0/16 any (msg:\"offset, depth, within test\"; flow:to_client; sid:2002; content:HTTP; depth:4; content:Server:; offset:15; within:100; depth:200;)");
    if (sig == NULL)
        return;
    prevsig->next = sig;
    prevsig = sig;

    sig = SigInit("alert ip 1.2.3.4 any -> any any (msg:\"Inliniac blog within test\"; flow:to_client; sid:2003; content:inliniac; content:blog; within:9;)");
    if (sig == NULL)
        return;
    prevsig->next = sig;
    prevsig = sig;

    sig = SigInit("alert ip 2001::1 any -> 2001::3 any (msg:\"abcdefg distance 1 test\"; flow:to_server; sid:2004; content:abcd; content:efgh; within:4; distance:0; content:ijkl; within:4; distance:0;)");
    if (sig == NULL)
        return;
    prevsig->next = sig;
    prevsig = sig;

    sig = SigInit("alert ip 2001::5 any -> 2001::7 any (msg:\"abcdef distance 0 test\"; flow:to_server; sid:2005; content:abcdef; content:ghijklmnop; distance:0;)");
    if (sig == NULL)
        return;
    prevsig->next = sig;
    prevsig = sig;


    sig = SigInit("alert ip 10.0.0.0/8 any -> 4.3.2.1 any (msg:\"abcdefg distance 1 test\"; flow:to_server; sid:2006; content:abcdef; content:ghijklmnop; distance:1;)");
    if (sig == NULL)
        return;
    prevsig->next = sig;
    prevsig = sig;

    sig = SigInit("alert tcp 172.16.1.0/24 any -> 0.0.0.0/0 any (msg:\"HTTP response code cap\"; flow:to_client; content:HTTP; depth:4; pcre:\"/^HTTP\\/\\d\\.\\d (?<http_response>[0-9]+) [A-z\\s]+\\r\\n/\"; depth:50; sid:3;)");
    if (sig == NULL)
        return;
    prevsig->next = sig;
    prevsig = sig;

    sig = SigInit("alert tcp 172.16.2.0/24 any -> 10.10.10.10 any (msg:\"HTTP server code cap\"; flow:to_client; content:Server:; depth:500; pcre:\"/^Server: (?<http_server>.*)\\r\\n/m\"; sid:4;)");
    if (sig == NULL)
        return;
    prevsig->next = sig;
    prevsig = sig;

    sig = SigInit("alert tcp 192.168.0.1 any -> 1.0.2.1 any (msg:\"\to_client nocase test\"; flow:to_client; content:Servere:; nocase; sid:400;)");
    if (sig == NULL)
        return;
    prevsig->next = sig;
    prevsig = sig;

    sig = SigInit("alert tcp 192.168.0.4 any -> 1.2.0.1 any (msg:\"HTTP UA code cap\"; flow:to_server; content:User-Agent:; depth:300; pcre:\"/^User-Agent: (?<http_ua>.*)\\r\\n/m\"; sid:5;)");
    if (sig == NULL)
        return;
    prevsig->next = sig;
    prevsig = sig;

    sig = SigInit("alert tcp 192.168.0.12 any -> 0.0.0.0/0 any (msg:\"HTTP http_host flowvar www.inliniac.net\"; flow:to_server; flowvar:http_host,\"www.inliniac.net\"; sid:7;)");
    if (sig) {
        prevsig->next = sig;
        prevsig = sig;
    }
    sig = SigInit("alert tcp 192.168.0.0/16 any -> 0.0.0.0/0 any (msg:\"HTTP http_uri flowvar MattJonkman\"; flow:to_server; flowvar:http_uri,\"MattJonkman\"; sid:8;)");
    if (sig) {
        prevsig->next = sig;
        prevsig = sig;
    }

    sig = SigInit("alert tcp 0.0.0.0/0 any -> 0.0.0.0/0 any (msg:\"HTTP uricontent VictorJulien\"; flow:to_server; uricontent:\"VJ\"; sid:9;)");
    if (sig) {
        prevsig->next = sig;
        prevsig = sig;
    }
*/
//#if 0
    int good = 0, bad = 0;
    FILE *fp = fopen("/etc/vips/rules/bleeding-all.rules", "r");
    //FILE *fp = fopen("/home/victor/rules/vips-http.sigs", "r");
    //FILE *fp = fopen("/home/victor/rules/emerging-dshield.rules", "r");
    //FILE *fp = fopen("/home/victor/rules/emerging-web-small.rules", "r");
    //FILE *fp = fopen("/home/victor/rules/web-misc.rules", "r");
    //FILE *fp = fopen("/home/victor/rules/vips-all.sigs", "r");
    //FILE *fp = fopen("/home/victor/rules/all.rules", "r");
    //FILE *fp = fopen("/home/victor/rules/all_iplists.rules", "r");
    //FILE *fp = fopen("/home/victor/rules/funky.rules", "r");
    //FILE *fp = fopen("/etc/vips/rules/zango.rules", "r");
    //FILE *fp = fopen("/home/victor/rules/vips-vrt-all.sigs", "r");
    //FILE *fp = fopen("/home/victor/rules/test-many-ips.rules", "r");
    if (fp == NULL) {
        printf("ERROR, could not open sigs file\n");
        exit(1);
    }
    char line[8192] = "";
    while(fgets(line, (int)sizeof(line), fp) != NULL) {
        if (line[0] == '\n' || line[0] == ' ' || line[0] == '#' || line[0] == '\t')
            continue;

        //if (i > 1000) break;

        sig = SigInit(line);
        if (sig) {
            prevsig->next = sig;
            prevsig = sig;
            good++;
        } else {
            bad++;
        }
    }
    fclose(fp);
    printf("SigLoadSignatures: %d successfully loaded from file. %d sigs failed to load\n", good, bad);

    /* http_ua -- for the log-httplog module */
    sig = SigInit("alert tcp any any -> any any (msg:\"HTTP UA cap\"; flow:to_server; content:\"User-Agent:\"; depth:400; pcre:\"/^User-Agent: (?P<http_ua>.*)\\r\\n/m\"; noalert; sid:4;)");
    if (sig == NULL)
        return;
    prevsig->next = sig;
    prevsig = sig;

//#endif

    /* Setup the signature group lookup structure and
     * pattern matchers */
    SigGroupBuild(sig_list);
abort();
}

/* check if a certain sid alerted, this is used in the test functions */
int PacketAlertCheck(Packet *p, u_int32_t sid)
{
    u_int16_t i = 0;
    int match = 0;

    for (i = 0; i < p->alerts.cnt; i++) {
        if (p->alerts.alerts[i].sid == sid)
            match++;
    }

    return match;
}

int PacketAlertAppend(Packet *p, u_int8_t gid, u_int32_t sid, u_int8_t rev, u_int8_t prio, char *msg)
{
    /* XXX overflow check? */

    p->alerts.alerts[p->alerts.cnt].gid = gid;
    p->alerts.alerts[p->alerts.cnt].sid = sid;
    p->alerts.alerts[p->alerts.cnt].rev = rev;
    p->alerts.alerts[p->alerts.cnt].prio = prio;
    p->alerts.alerts[p->alerts.cnt].msg = msg;
    p->alerts.cnt++;

    return 0;
}

int SigMatchSignatures(ThreadVars *th_v, PatternMatcherThread *pmt, Packet *p)
{
    int match = 0, fmatch = 0;
    Signature *s = NULL;
    SigMatch *sm = NULL;
    SigGroupContainer *sg = NULL;

    /* we assume we don't have an uri when we start inspection */
    pmt->de_have_httpuri = 0;
    pmt->de_scanned_httpuri = 0;

    /* find the right mpm instance */
    DetectAddressGroup *g = DetectAddressLookupGroup(g_src_gh,&p->src);
    if (g != NULL) {
        /* source group found, lets try a dst group */
        g = DetectAddressLookupGroup(g->dst_gh,&p->dst);
        if (g != NULL) {
            pmt->mc = g->sh->mpm_ctx;
            pmt->mcu = g->sh->mpm_uri_ctx;

//            printf("SigMatchSignatures: mc %p, mcu %p\n", pmt->mc, pmt->mcu);

            /* point this sig list to sg */
            sg = g->sh->head;
            //printf("sigs %u\n", g->sh->sig_cnt);
        }
    }

    if (pmt->mc != NULL) {
        /* run the pattern matcher against the packet */
        //u_int32_t cnt = 
        PacketPatternMatch(th_v, pmt, p);
        //printf("cnt %u\n", cnt);
    }

    /* inspect the sigs against the packet */
    for ( ; sg != NULL; sg = sg->next) {
        s = sg->s;

        /* XXX maybe a (re)set function? */
        pmt->pkt_ptr = NULL;
        pmt->pkt_off = 0;

        if (s->flags & SIG_FLAG_RECURSIVE) {
            u_int8_t rmatch = 0;
            pmt->pkt_cnt = 0;

            do {
                sm = s->match;
                while (sm) {
                    match = sigmatch_table[sm->type].Match(th_v, pmt, p, s, sm);
                    if (match) {
                        /* okay, try the next match */
                        sm = sm->next;

                        /* only if the last matched as well, we have a hit */
                        if (sm == NULL) {
                            if (!(s->flags & SIG_FLAG_NOALERT)) {
                                /* only add once */
                                if (rmatch == 0) {
                                    PacketAlertAppend(p, 1, s->id, s->rev, s->prio, s->msg);

                                    /* set verdict on packet */
                                    p->action = s->action;
                                }
                            }
                            rmatch = fmatch = 1;
                            pmt->pkt_cnt++;
                        }
                    } else {
                        /* done with this sig */
                        sm = NULL;
                        rmatch = 0;
                    }
                }
                /* Limit the number of times we do this recursive thing.
                 * XXX is this a sane limit? Should it be configurable? */
                if (pmt->pkt_cnt == 10)
                    break;
            } while (rmatch);
        } else {
            sm = s->match;
            while (sm) {
                match = sigmatch_table[sm->type].Match(th_v, pmt, p, s, sm);
                if (match) {
                    /* okay, try the next match */
                    sm = sm->next;

                    /* only if the last matched as well, we have a hit */
                    if (sm == NULL) {
                        //printf("Signature %u matched: %s\n", s->id, s->msg ? s->msg : "");
                        fmatch = 1;

                        if (!(s->flags & SIG_FLAG_NOALERT)) {
                            PacketAlertAppend(p, 1, s->id, s->rev, s->prio, s->msg);

                            /* set verdict on packet */
                            p->action = s->action;
                        }
                    }
                } else {
                    /* done with this sig */
                    sm = NULL;
                }
            }
        }
    }

    /* cleanup pkt specific part of the patternmatcher */
    PacketPatternCleanup(th_v, pmt);
    return fmatch;
}

/* tm module api functions */
int Detect(ThreadVars *t, Packet *p, void *data) {
    PatternMatcherThread *pmt = (PatternMatcherThread *)data;

    return SigMatchSignatures(t,pmt,p);
}

int DetectThreadInit(ThreadVars *t, void **data) {
    return PatternMatcherThreadInit(t,data);
}

int DetectThreadDeinit(ThreadVars *t, void *data) {
    return PatternMatcherThreadDeinit(t,data);
}

void SigCleanSignatures()
{
    Signature *s = NULL, *ns;

    for (s = sig_list; s != NULL;) {
        ns = s->next;
        SigFree(s);
        s = ns;
    }
}


/* add all signatures to their own source address group
 * XXX not currently used */
int SigAddressPrepareStage1(Signature *s) {
    Signature *tmp_s = NULL;
    DetectAddressGroup *gr = NULL;
    u_int32_t cnt = 0, sigs = 0;

    printf("* Building signature grouping structure, stage 1: adding signatures to signature source addresses...\n");

    /* now for every rule add the source group */
    for (tmp_s = s; tmp_s != NULL; tmp_s = tmp_s->next) {
        for (gr = tmp_s->src.ipv4_head; gr != NULL; gr = gr->next) {
            if (SigGroupAppend(gr,tmp_s) < 0) {
                goto error;
            }
            cnt++;
        }
        for (gr = tmp_s->src.ipv6_head; gr != NULL; gr = gr->next) {
            if (SigGroupAppend(gr,tmp_s) < 0) {
                goto error;
            }
            cnt++;
        }
        for (gr = tmp_s->src.any_head; gr != NULL; gr = gr->next) {
            if (SigGroupAppend(gr,tmp_s) < 0) {
                goto error;
            }
            cnt++;
        }
        sigs++;
    }
    //DetectSigGroupPrintMemory();
    printf("* %u signatures appended %u times to the source addresses\n", sigs, cnt);
    printf("* Building signature grouping structure, stage 1: adding signatures to signature source addresses... done\n");

    return 0;
error:
    printf("SigAddressPrepareStage1 error\n");
    return -1;
}

/* fill the global src group head, with the sigs included */
int SigAddressPrepareStage2(Signature *s) {
    Signature *tmp_s = NULL;
    DetectAddressGroup *gr = NULL, *lookup_gr = NULL;
    u_int32_t cnt = 0, sigs = 0, insert = 0;

    printf("* Building signature grouping structure, stage 2: building source address list\n");

    g_src_gh = DetectAddressGroupsHeadInit();
    if (g_src_gh == NULL) {
        goto error;
    }
    g_tmp_gh = DetectAddressGroupsHeadInit();
    if (g_tmp_gh == NULL) {
        goto error;
    }

    /* now for every rule add the source group to our temp list */
    for (tmp_s = s; tmp_s != NULL; tmp_s = tmp_s->next) {
        for (gr = tmp_s->src.ipv4_head; gr != NULL; gr = gr->next) {
            if ((lookup_gr = DetectAddressGroupLookup(g_tmp_gh->ipv4_head,gr->ad)) == NULL) {
                DetectAddressGroup *grtmp = DetectAddressGroupInit();
                if (grtmp == NULL) {
                    goto error;
                }
                DetectAddressData *adtmp = DetectAddressDataCopy(gr->ad);
                if (adtmp == NULL) {
                    goto error;
                }
                grtmp->ad = adtmp;
                DetectAddressGroupAdd(&g_tmp_gh->ipv4_head,grtmp);

                SigGroupAppend(grtmp,tmp_s);
                cnt++;
                insert++;
            } else {
                /* our group will only have one sig, this one. So add that. */
                SigGroupAppend(lookup_gr,tmp_s);
                cnt++;
            }
        }
        for (gr = tmp_s->src.ipv6_head; gr != NULL; gr = gr->next) {
            if ((lookup_gr = DetectAddressGroupLookup(g_tmp_gh->ipv6_head,gr->ad)) == NULL) {
                DetectAddressGroup *grtmp = DetectAddressGroupInit();
                if (grtmp == NULL) {
                    goto error;
                }
                DetectAddressData *adtmp = DetectAddressDataCopy(gr->ad);
                if (adtmp == NULL) {
                    goto error;
                }
                grtmp->ad = adtmp;
                DetectAddressGroupAdd(&g_tmp_gh->ipv6_head,grtmp);

                SigGroupAppend(grtmp,tmp_s);
                cnt++;
                insert++;
            } else {
                /* our group will only have one sig, this one. So add that. */
                SigGroupAppend(lookup_gr,tmp_s);
                cnt++;
            }
        }
        /* XXX review 'any' usage here */
        for (gr = tmp_s->src.any_head; gr != NULL; gr = gr->next) {
            if ((lookup_gr = DetectAddressGroupLookup(g_tmp_gh->any_head,gr->ad)) == NULL) {
                DetectAddressGroup *grtmp = DetectAddressGroupInit();
                if (grtmp == NULL) {
                    goto error;
                }
                DetectAddressData *adtmp = DetectAddressDataCopy(gr->ad);
                if (adtmp == NULL) {
                    goto error;
                }
                grtmp->ad = adtmp;
                DetectAddressGroupAdd(&g_tmp_gh->any_head,grtmp);

                SigGroupAppend(grtmp,tmp_s);
                cnt++;
                insert++;
            } else {
                /* our group will only have one sig, this one. So add that. */
                SigGroupAppend(lookup_gr,tmp_s);
                cnt++;
            }
        }
        sigs++;
    }

    //DetectAddressGroupPrintMemory();
    //DetectSigGroupPrintMemory();

    //printf("g_tmp_gh strt\n");
    //DetectAddressGroupPrintList(g_tmp_gh->ipv4_head);
    //printf("g_tmp_gh end\n");

    for (gr = g_tmp_gh->ipv4_head; gr != NULL; ) {
        //printf("Inserting2'ing: "); DetectAddressDataPrint(gr->ad);
        DetectAddressGroup *grnext = gr->next;

        gr->next = NULL;
        if (DetectAddressGroupInsert(g_src_gh,gr) < 0)
            goto error;

        gr = grnext;
    }
    for (gr = g_tmp_gh->ipv6_head; gr != NULL; ) {
        //printf("Inserting2'ing: "); DetectAddressDataPrint(gr->ad);
        DetectAddressGroup *grnext = gr->next;

        gr->next = NULL;
        if (DetectAddressGroupInsert(g_src_gh,gr) < 0)
            goto error;

        gr = grnext;
    }
    /* XXX whats the point of the any temp list if any is always just
     * one object.... ??? */
    for (gr = g_tmp_gh->any_head; gr != NULL; ) {
        //printf("Inserting2'ing: "); DetectAddressDataPrint(gr->ad);
        DetectAddressGroup *grnext = gr->next;

        gr->next = NULL;
        if (DetectAddressGroupInsert(g_src_gh,gr) < 0)
            goto error;

        gr = grnext;
    }



    //DetectAddressGroupPrintMemory();
    //DetectSigGroupPrintMemory();

    //printf("g_src_gh strt\n");
    //DetectAddressGroupPrintList(g_src_gh->ipv4_head);
    //printf("g_src_gh end\n");

    u_int32_t cnt_any = 0, cnt_ipv4 = 0, cnt_ipv6 = 0;

    for (gr = g_src_gh->any_head; gr != NULL; gr = gr->next) {
       cnt_any++;
    }
    for (gr = g_src_gh->ipv4_head; gr != NULL; gr = gr->next) {
       cnt_ipv4++;
    }
    for (gr = g_src_gh->ipv6_head; gr != NULL; gr = gr->next) {
       cnt_ipv6++;
    }

    printf("* %u signatures, %u sigs appends, %u actual source address inserts\n", sigs,cnt,insert);

    printf("* Source any: %u address blocks.\n", cnt_any);
    printf("* Source ipv4: %u address blocks.\n", cnt_ipv4);
    printf("* Source ipv6: %u address blocks.\n", cnt_ipv6);
    printf("* Building signature grouping structure, stage 2: building source address list... done\n");

    return 0;
error:
    printf("SigAddressPrepareStage2 error\n");
    return -1;
}

int SigAddressPrepareStage3(Signature *s) {
    Signature *tmp_s = NULL;
    DetectAddressGroup *gr = NULL, *sgr = NULL, *lookup_gr = NULL;
    SigGroupContainer *sgc = NULL;
    u_int32_t cnt = 0, copy = 0, real = 0, mpm_unique = 0, mpm_reuse = 0, mpm_none = 0,
              mpm_uri_unique = 0, mpm_uri_reuse = 0, mpm_uri_none = 0;

    printf("* Building signature grouping structure, stage 3: building destination address lists...\n");

    SigGroupHeadListClean();

    /* loop through the global source address list */
    for (gr = g_src_gh->ipv4_head; gr != NULL; gr = gr->next) {
        //printf(" * Source group: "); DetectAddressDataPrint(gr->ad); printf("\n");

        /* use a tmp list for speeding up insertions */
        DetectAddressGroupsHead *tmp_gh = DetectAddressGroupsHeadInit();
        if (tmp_gh == NULL) {
            goto error;
        }

        /* loop through all signatures in this source address group */
        for (sgc = gr->sh->head; sgc != NULL; sgc = sgc->next) {
            tmp_s = sgc->s;

            /* build the temp list */
            for (sgr = tmp_s->dst.ipv4_head; sgr != NULL; sgr = sgr->next) {
                if ((lookup_gr = DetectAddressGroupLookup(tmp_gh->ipv4_head,sgr->ad)) == NULL) {
                    DetectAddressGroup *grtmp = DetectAddressGroupInit();
                    if (grtmp == NULL) {
                        goto error;
                    }
                    DetectAddressData *adtmp = DetectAddressDataCopy(sgr->ad);
                    if (adtmp == NULL) {
                        goto error;
                    }
                    grtmp->ad = adtmp;
                    DetectAddressGroupAdd(&tmp_gh->ipv4_head,grtmp);

                    SigGroupAppend(grtmp,tmp_s);
                } else {
                    /* our group will only have one sig, this one. So add that. */
                    SigGroupAppend(lookup_gr,tmp_s);
                }
            }
        }

        /* for each address in the tmp list, insert a copy */
        for (sgr = tmp_gh->ipv4_head; sgr != NULL; sgr = sgr->next) {
            DetectAddressGroup *grtmp = DetectAddressGroupInit();
            if (grtmp == NULL) {
                goto error;
            }
            DetectAddressData *adtmp = DetectAddressDataCopy(sgr->ad);
            if (adtmp == NULL) {
                goto error;
            }
            grtmp->ad = adtmp;

            SigGroupListCopyAppend(sgr,grtmp);

            if (gr->dst_gh == NULL) {
                gr->dst_gh = DetectAddressGroupsHeadInit();
            }

            int r = DetectAddressGroupInsert(gr->dst_gh,grtmp);
            if (r < 0) goto error;
        }

        /* see if the sig group head of each address group is the
         * same as an earlier one. If it is, free our head and use
         * a pointer to the earlier one. This saves _a lot_ of memory.
         */
        for (sgr = gr->dst_gh->ipv4_head; sgr != NULL; sgr = sgr->next) {
            /* Because a pattern matcher context uses quite some
             * memory, we first check if we can reuse it from
             * another group head. */
            SigGroupHead *sgh = SigGroupHeadListGet(sgr->sh);
            if (sgh == NULL) {
                /* put the contents in our head */
                SigGroupContentLoad(sgr->sh);
                SigGroupUricontentLoad(sgr->sh);

                //SigGroupContentListPrint(sgr->sh);
                //SigGroupUricontentListPrint(sgr->sh);

                if (sgr->sh->content_size == 0) {
                    mpm_none++;
                } else {
                    /* now have a look if we can reuse a mpm ctx */
                    SigGroupHead *mpmsh = SigGroupHeadListGetMpm(sgr->sh);
                    if (mpmsh == NULL) {
                        mpm_unique++;
                    } else {
                        mpm_reuse++;

                        sgr->sh->mpm_ctx = mpmsh->mpm_ctx;
                        sgr->sh->flags |= SIG_GROUP_HEAD_MPM_COPY;
                        SigGroupListContentClean(sgr->sh);
                    }
                }

                if (sgr->sh->uri_content_size == 0) {
                    mpm_uri_none++;
                } else {
                    /* now have a look if we can reuse a uri mpm ctx */
                    SigGroupHead *mpmsh = SigGroupHeadListGetMpmUri(sgr->sh);
                    if (mpmsh == NULL) {
                        mpm_uri_unique++;
                    } else {
                        mpm_uri_reuse++;

                        sgr->sh->mpm_uri_ctx = mpmsh->mpm_uri_ctx;
                        sgr->sh->flags |= SIG_GROUP_HEAD_MPM_URI_COPY;
                        SigGroupListUricontentClean(sgr->sh);
                    }
                }

                /* init the pattern matcher, this will respect the copy
                 * setting */
                if (PatternMatchPrepareGroup(sgr->sh) < 0)
                    goto error;

                SigGroupHeadAppend(sgr->sh);
                real++;
            } else {
                SigGroupHeadFree(sgr->sh);
                sgr->sh = sgh;
                copy++;
                sgr->flags |= SIG_GROUP_COPY;
            }
        }

        /* free the temp list */
        DetectAddressGroupsHeadFree(tmp_gh);
        /* clear now unneeded sig group head */
        SigGroupHeadFree(gr->sh);
        gr->sh = NULL;

        DetectAddressGroup *tgr;
        for (tgr = gr->dst_gh->ipv4_head; tgr != NULL; tgr = tgr->next) {
            cnt++;
        }

        //printf(" * Source group: "); DetectAddressDataPrint(gr->ad); printf(": %d destination groups.\n", cnt);
        cnt = 0;
    }
    for (gr = g_src_gh->ipv6_head; gr != NULL; gr = gr->next) {
        //printf(" * Source group: "); DetectAddressDataPrint(gr->ad); printf("\n");

        /* use a tmp list for speeding up insertions */
        DetectAddressGroupsHead *tmp_gh = DetectAddressGroupsHeadInit();
        if (tmp_gh == NULL) {
            goto error;
        }

        /* loop through all signatures in this source address group */
        for (sgc = gr->sh->head; sgc != NULL; sgc = sgc->next) {
            tmp_s = sgc->s;

            /* build the temp list */
            for (sgr = tmp_s->dst.ipv6_head; sgr != NULL; sgr = sgr->next) {
                if ((lookup_gr = DetectAddressGroupLookup(tmp_gh->ipv6_head,sgr->ad)) == NULL) {
                    DetectAddressGroup *grtmp = DetectAddressGroupInit();
                    if (grtmp == NULL) {
                        goto error;
                    }
                    DetectAddressData *adtmp = DetectAddressDataCopy(sgr->ad);
                    if (adtmp == NULL) {
                        goto error;
                    }
                    grtmp->ad = adtmp;
                    DetectAddressGroupAdd(&tmp_gh->ipv6_head,grtmp);

                    SigGroupAppend(grtmp,tmp_s);
                } else {
                    /* our group will only have one sig, this one. So add that. */
                    SigGroupAppend(lookup_gr,tmp_s);
                }
            }
        }

        /* for each address in the tmp list, insert a copy */
        for (sgr = tmp_gh->ipv6_head; sgr != NULL; sgr = sgr->next) {
            DetectAddressGroup *grtmp = DetectAddressGroupInit();
            if (grtmp == NULL) {
                goto error;
            }
            DetectAddressData *adtmp = DetectAddressDataCopy(sgr->ad);
            if (adtmp == NULL) {
                goto error;
            }
            grtmp->ad = adtmp;

            SigGroupListCopyAppend(sgr,grtmp);

            if (gr->dst_gh == NULL) {
                gr->dst_gh = DetectAddressGroupsHeadInit();
            }

            int r = DetectAddressGroupInsert(gr->dst_gh,grtmp);
            if (r < 0) goto error;
        }

        /* see if the sig group head of each address group is the
         * same as an earlier one. If it is, free our head and use
         * a pointer to the earlier one. This saves _a lot_ of memory.
         */
        for (sgr = gr->dst_gh->ipv6_head; sgr != NULL; sgr = sgr->next) {
            /* Because a pattern matcher context uses quite some
             * memory, we first check if we can reuse it from
             * another group head. */
            SigGroupHead *sgh = SigGroupHeadListGet(sgr->sh);
            if (sgh == NULL) {
                /* put the contents in our head */
                SigGroupContentLoad(sgr->sh);
                SigGroupUricontentLoad(sgr->sh);

                //SigGroupContentListPrint(sgr->sh);
                //SigGroupUricontentListPrint(sgr->sh);

                if (sgr->sh->content_size == 0) {
                    mpm_none++;
                } else {
                    /* now have a look if we can reuse a mpm ctx */
                    SigGroupHead *mpmsh = SigGroupHeadListGetMpm(sgr->sh);
                    if (mpmsh == NULL) {
                        mpm_unique++;
                    } else {
                        mpm_reuse++;

                        sgr->sh->mpm_ctx = mpmsh->mpm_ctx;
                        sgr->sh->flags |= SIG_GROUP_HEAD_MPM_COPY;
                        SigGroupListContentClean(sgr->sh);
                    }
                }

                if (sgr->sh->uri_content_size == 0) {
                    mpm_uri_none++;
                } else {
                    /* now have a look if we can reuse a uri mpm ctx */
                    SigGroupHead *mpmsh = SigGroupHeadListGetMpmUri(sgr->sh);
                    if (mpmsh == NULL) {
                        mpm_uri_unique++;
                    } else {
                        mpm_uri_reuse++;

                        sgr->sh->mpm_uri_ctx = mpmsh->mpm_uri_ctx;
                        sgr->sh->flags |= SIG_GROUP_HEAD_MPM_URI_COPY;
                        SigGroupListUricontentClean(sgr->sh);
                    }
                }

                /* init the pattern matcher, this will respect the copy
                 * setting */
                if (PatternMatchPrepareGroup(sgr->sh) < 0)
                    goto error;

                SigGroupHeadAppend(sgr->sh);
                real++;
            } else {
                SigGroupHeadFree(sgr->sh);
                sgr->sh = sgh;
                copy++;
                sgr->flags |= SIG_GROUP_COPY;
            }
        }

        /* free the temp list */
        DetectAddressGroupsHeadFree(tmp_gh);
        /* clear now unneeded sig group head */
        SigGroupHeadFree(gr->sh);
        gr->sh = NULL;

        DetectAddressGroup *tgr;
        for (tgr = gr->dst_gh->ipv6_head; tgr != NULL; tgr = tgr->next) {
            cnt++;
        }

        //printf(" * Source group: "); DetectAddressDataPrint(gr->ad); printf(": %d destination groups.\n", cnt);
        cnt = 0;
    }
    for (gr = g_src_gh->any_head; gr != NULL; gr = gr->next) {
        //printf(" * Source group: "); DetectAddressDataPrint(gr->ad); printf("\n");

        /* use a tmp list for speeding up insertions */
        DetectAddressGroupsHead *tmp_gh = DetectAddressGroupsHeadInit();
        if (tmp_gh == NULL) {
            goto error;
        }

        /* loop through all signatures in this source address group */
        for (sgc = gr->sh->head; sgc != NULL; sgc = sgc->next) {
            tmp_s = sgc->s;

            /* build the temp list */
            for (sgr = tmp_s->dst.any_head; sgr != NULL; sgr = sgr->next) {
                if ((lookup_gr = DetectAddressGroupLookup(tmp_gh->any_head,sgr->ad)) == NULL) {
                    DetectAddressGroup *grtmp = DetectAddressGroupInit();
                    if (grtmp == NULL) {
                        goto error;
                    }
                    DetectAddressData *adtmp = DetectAddressDataCopy(sgr->ad);
                    if (adtmp == NULL) {
                        goto error;
                    }
                    grtmp->ad = adtmp;
                    DetectAddressGroupAdd(&tmp_gh->any_head,grtmp);

                    SigGroupAppend(grtmp,tmp_s);
                } else {
                    /* our group will only have one sig, this one. So add that. */
                    SigGroupAppend(lookup_gr,tmp_s);
                }
            }
        }

        /* for each address in the tmp list, insert a copy */
        for (sgr = tmp_gh->any_head; sgr != NULL; sgr = sgr->next) {
            DetectAddressGroup *grtmp = DetectAddressGroupInit();
            if (grtmp == NULL) {
                goto error;
            }
            DetectAddressData *adtmp = DetectAddressDataCopy(sgr->ad);
            if (adtmp == NULL) {
                goto error;
            }
            grtmp->ad = adtmp;

            SigGroupListCopyAppend(sgr,grtmp);

            if (gr->dst_gh == NULL) {
                gr->dst_gh = DetectAddressGroupsHeadInit();
            }

            int r = DetectAddressGroupInsert(gr->dst_gh,grtmp);
            if (r < 0) goto error;
        }

        /* see if the sig group head of each address group is the
         * same as an earlier one. If it is, free our head and use
         * a pointer to the earlier one. This saves _a lot_ of memory.
         */
        for (sgr = gr->dst_gh->any_head; sgr != NULL; sgr = sgr->next) {
            /* Because a pattern matcher context uses quite some
             * memory, we first check if we can reuse it from
             * another group head. */
            SigGroupHead *sgh = SigGroupHeadListGet(sgr->sh);
            if (sgh == NULL) {
                /* put the contents in our head */
                SigGroupContentLoad(sgr->sh);
                SigGroupUricontentLoad(sgr->sh);

                //SigGroupContentListPrint(sgr->sh);
                //SigGroupUricontentListPrint(sgr->sh);

                if (sgr->sh->content_size == 0) {
                    mpm_none++;
                } else {
                    /* now have a look if we can reuse a mpm ctx */
                    SigGroupHead *mpmsh = SigGroupHeadListGetMpm(sgr->sh);
                    if (mpmsh == NULL) {
                        mpm_unique++;
                    } else {
                        mpm_reuse++;

                        sgr->sh->mpm_ctx = mpmsh->mpm_ctx;
                        sgr->sh->flags |= SIG_GROUP_HEAD_MPM_COPY;
                        SigGroupListContentClean(sgr->sh);
                    }
                }

                if (sgr->sh->uri_content_size == 0) {
                    mpm_uri_none++;
                } else {
                    /* now have a look if we can reuse a uri mpm ctx */
                    SigGroupHead *mpmsh = SigGroupHeadListGetMpmUri(sgr->sh);
                    if (mpmsh == NULL) {
                        mpm_uri_unique++;
                    } else {
                        mpm_uri_reuse++;

                        sgr->sh->mpm_uri_ctx = mpmsh->mpm_uri_ctx;
                        sgr->sh->flags |= SIG_GROUP_HEAD_MPM_URI_COPY;
                        SigGroupListUricontentClean(sgr->sh);
                    }
                }

                /* init the pattern matcher, this will respect the copy
                 * setting */
                if (PatternMatchPrepareGroup(sgr->sh) < 0)
                    goto error;

                SigGroupHeadAppend(sgr->sh);
                real++;
            } else {
                SigGroupHeadFree(sgr->sh);
                sgr->sh = sgh;
                copy++;
                sgr->flags |= SIG_GROUP_COPY;
            }
        }

        /* free the temp list */
        DetectAddressGroupsHeadFree(tmp_gh);
        /* clear now unneeded sig group head */
        SigGroupHeadFree(gr->sh);
        gr->sh = NULL;

        DetectAddressGroup *tgr;
        for (tgr = gr->dst_gh->any_head; tgr != NULL; tgr = tgr->next) {
            cnt++;
        }

        //printf(" * Source group: "); DetectAddressDataPrint(gr->ad); printf(": %d destination groups.\n", cnt);
        cnt = 0;
    }

    /* XXX cleanup group head (uri)content_array's */
    SigGroupHeadFreeMpmArrays();

    //DetectAddressGroupPrintMemory();
    //DetectSigGroupPrintMemory();

    printf("* Signature group heads: unique %u, copies %u.\n", real,copy);
    printf("* MPM instances: %u unique, copies %u (none %u).\n", mpm_unique, mpm_reuse, mpm_none);
    printf("* MPM (URI) instances: %u unique, copies %u (none %u).\n", mpm_uri_unique, mpm_uri_reuse, mpm_uri_none);
    printf("* Building signature grouping structure, stage 3: building destination address lists... done\n");

    return 0;
error:
    printf("SigAddressPrepareStage3 error\n");
    return -1;
}

int SigAddressCleanupStage1(void) {
    DetectAddressGroup *global_src_gr = NULL;

    printf("* Cleaning up signature grouping structure, stage 1...\n");

    for (global_src_gr = g_src_gh->ipv4_head; global_src_gr != NULL;
         global_src_gr = global_src_gr->next)
    {
        DetectAddressGroupsHeadCleanup(global_src_gr->dst_gh);
    }

    for (global_src_gr = g_src_gh->ipv6_head; global_src_gr != NULL;
         global_src_gr = global_src_gr->next)
    {
        DetectAddressGroupsHeadCleanup(global_src_gr->dst_gh);
    }

    for (global_src_gr = g_src_gh->any_head; global_src_gr != NULL;
         global_src_gr = global_src_gr->next)
    {
        DetectAddressGroupsHeadCleanup(global_src_gr->dst_gh);
    }

    DetectAddressGroupsHeadCleanup(g_src_gh);

    printf("* Cleaning up signature grouping structure, stage 1... done\n");
    return 0;
}

/* shortcut for debugging. If enabled Stage5 will
 * print sigid's for all groups */
//#define PRINTSIGS

/* just printing */
int SigAddressPrepareStage5(void) {
    DetectAddressGroupsHead *global_dst_gh = NULL;
    DetectAddressGroup *global_src_gr = NULL, *global_dst_gr = NULL;

    printf("* Building signature grouping structure, stage 5: print...\n");

    for (global_src_gr = g_src_gh->ipv4_head; global_src_gr != NULL;
         global_src_gr = global_src_gr->next)
    {
        printf("- "); DetectAddressDataPrint(global_src_gr->ad);

        global_dst_gh = global_src_gr->dst_gh;
        if (global_dst_gh == NULL)
            continue;

        for (global_dst_gr = global_dst_gh->ipv4_head;
             global_dst_gr != NULL;
             global_dst_gr = global_dst_gr->next)
        {
            printf(" - [%4u] ", global_dst_gr->sh ? global_dst_gr->sh->sig_cnt : 0); DetectAddressDataPrint(global_dst_gr->ad);
            if (global_dst_gr->sh) {
                if (global_dst_gr->sh->flags & SIG_GROUP_INITIALIZED) {
                    printf("  - INITIALIZED ");
                    if (global_dst_gr->sh->flags & SIG_GROUP_COPY) {
                        printf("(COPY)\n");
                    } else {
                        printf("\n");
                    }
                }
            }
#ifdef PRINTSIGS
            if (global_dst_gr->sh && global_dst_gr->sh->head) {
                printf ("  - ");
                SigGroupContainer *sg; 
                for (sg = global_dst_gr->sh->head; sg != NULL; sg = sg->next) {
                    printf("%u", sg->s->id);
                    if (sg->next) printf(",");
                    else printf("\n");
                }
            }
#endif
        }
        for (global_dst_gr = global_dst_gh->any_head;
             global_dst_gr != NULL;
             global_dst_gr = global_dst_gr->next)
        {
            printf(" - [%4u] ", global_dst_gr->sh ? global_dst_gr->sh->sig_cnt : 0); DetectAddressDataPrint(global_dst_gr->ad);
            if (global_dst_gr->sh) {
                if (global_dst_gr->sh->flags & SIG_GROUP_INITIALIZED) {
                    printf("  - INITIALIZED ");
                    if (global_dst_gr->sh->flags & SIG_GROUP_COPY) {
                        printf("(COPY)\n");
                    } else {
                        printf("\n");
                    }
                }
            }
#ifdef PRINTSIGS
            if (global_dst_gr->sh && global_dst_gr->sh->head) {
                printf ("  - ");
                SigGroupContainer *sg; 
                for (sg = global_dst_gr->sh->head; sg != NULL; sg = sg->next) {
                    printf("%u", sg->s->id);
                    if (sg->next) printf(",");
                    else printf("\n");
                }
            }
#endif
        }
    }

    for (global_src_gr = g_src_gh->ipv6_head; global_src_gr != NULL;
         global_src_gr = global_src_gr->next)
    {
        printf("- "); DetectAddressDataPrint(global_src_gr->ad);

        global_dst_gh = global_src_gr->dst_gh;
        if (global_dst_gh == NULL)
            continue;

        for (global_dst_gr = global_dst_gh->ipv6_head;
             global_dst_gr != NULL;
             global_dst_gr = global_dst_gr->next)
        {
            printf(" - [%4u] ", global_dst_gr->sh ? global_dst_gr->sh->sig_cnt : 0); DetectAddressDataPrint(global_dst_gr->ad);
            if (global_dst_gr->sh) {
                if (global_dst_gr->sh->flags & SIG_GROUP_INITIALIZED) {
                    printf("  - INITIALIZED ");
                    if (global_dst_gr->sh->flags & SIG_GROUP_COPY) {
                        printf("(COPY)\n");
                    } else {
                        printf("\n");
                    }
                }
            }
#ifdef PRINTSIGS
            if (global_dst_gr->sh && global_dst_gr->sh->head) {
                printf ("  - ");
                SigGroupContainer *sg; 
                for (sg = global_dst_gr->sh->head; sg != NULL; sg = sg->next) {
                    printf("%u", sg->s->id);
                    if (sg->next) printf(",");
                    else printf("\n");
                }
            }
#endif
        }
        for (global_dst_gr = global_dst_gh->any_head;
             global_dst_gr != NULL;
             global_dst_gr = global_dst_gr->next)
        {
            printf(" - [%4u] ", global_dst_gr->sh ? global_dst_gr->sh->sig_cnt : 0); DetectAddressDataPrint(global_dst_gr->ad);
            if (global_dst_gr->sh) {
                if (global_dst_gr->sh->flags & SIG_GROUP_INITIALIZED) {
                    printf("  - INITIALIZED ");
                    if (global_dst_gr->sh->flags & SIG_GROUP_COPY) {
                        printf("(COPY)\n");
                    } else {
                        printf("\n");
                    }
                }
            }
#ifdef PRINTSIGS
            if (global_dst_gr->sh && global_dst_gr->sh->head) {
                printf ("  - ");
                SigGroupContainer *sg; 
                for (sg = global_dst_gr->sh->head; sg != NULL; sg = sg->next) {
                    printf("%u", sg->s->id);
                    if (sg->next) printf(",");
                    else printf("\n");
                }
            }
#endif
        }
    }

    for (global_src_gr = g_src_gh->any_head; global_src_gr != NULL;
         global_src_gr = global_src_gr->next)
    {
        printf("- "); DetectAddressDataPrint(global_src_gr->ad);

        global_dst_gh = global_src_gr->dst_gh;
        if (global_dst_gh == NULL)
            continue;

        for (global_dst_gr = global_dst_gh->any_head;
             global_dst_gr != NULL;
             global_dst_gr = global_dst_gr->next)
        {
            printf(" - [%4u] ", global_dst_gr->sh ? global_dst_gr->sh->sig_cnt : 0); DetectAddressDataPrint(global_dst_gr->ad);
            if (global_dst_gr->sh) {
                if (global_dst_gr->sh->flags & SIG_GROUP_INITIALIZED) {
                    printf("  - INITIALIZED ");
                    if (global_dst_gr->sh->flags & SIG_GROUP_COPY) {
                        printf("(COPY)\n");
                    } else {
                        printf("\n");
                    }
                }
            }
#ifdef PRINTSIGS
            if (global_dst_gr->sh && global_dst_gr->sh->head) {
                printf ("  - ");
                SigGroupContainer *sg; 
                for (sg = global_dst_gr->sh->head; sg != NULL; sg = sg->next) {
                    printf("%u", sg->s->id);
                    if (sg->next) printf(",");
                    else printf("\n");
                }
            }
#endif
        } 
        for (global_dst_gr = global_dst_gh->ipv4_head;
             global_dst_gr != NULL;
             global_dst_gr = global_dst_gr->next)
        {
            printf(" - [%4u] ", global_dst_gr->sh ? global_dst_gr->sh->sig_cnt : 0); DetectAddressDataPrint(global_dst_gr->ad);
            if (global_dst_gr->sh) {
                if (global_dst_gr->sh->flags & SIG_GROUP_INITIALIZED) {
                    printf("  - INITIALIZED ");
                    if (global_dst_gr->sh->flags & SIG_GROUP_COPY) {
                        printf("(COPY)\n");
                    } else {
                        printf("\n");
                    }
                }
            }
#ifdef PRINTSIGS
            if (global_dst_gr->sh && global_dst_gr->sh->head) {
                printf ("  - ");
                SigGroupContainer *sg; 
                for (sg = global_dst_gr->sh->head; sg != NULL; sg = sg->next) {
                    printf("%u", sg->s->id);
                    if (sg->next) printf(",");
                    else printf("\n");
                }
            }
#endif
        }
        for (global_dst_gr = global_dst_gh->ipv6_head;
             global_dst_gr != NULL;
             global_dst_gr = global_dst_gr->next)
        {
            printf(" - [%4u] ", global_dst_gr->sh->sig_cnt); DetectAddressDataPrint(global_dst_gr->ad);
            if (global_dst_gr->sh) {
                if (global_dst_gr->sh->flags & SIG_GROUP_INITIALIZED) {
                    printf("  - INITIALIZED ");
                    if (global_dst_gr->sh->flags & SIG_GROUP_COPY) {
                        printf("(COPY)\n");
                    } else {
                        printf("\n");
                    }
                }
            }
#ifdef PRINTSIGS
            if (global_dst_gr->sh && global_dst_gr->sh->head) {
                printf ("  - ");
                SigGroupContainer *sg; 
                for (sg = global_dst_gr->sh->head; sg != NULL; sg = sg->next) {
                    printf("%u", sg->s->id);
                    if (sg->next) printf(",");
                    else printf("\n");
                }
            }
#endif
        }
    }
    printf("* Building signature grouping structure, stage 5: print... done\n");
    return 0;
}

int SigGroupBuild (Signature *s) {
    SigAddressPrepareStage1(s);
    SigAddressPrepareStage2(s);
    SigAddressPrepareStage3(s);
    return 0;
}

int SigGroupCleanup (void) {
    SigAddressCleanupStage1();
    return 0;
}

int SigGroupGetSrcAddress(DetectAddressGroupsHead *src) {
    u_int32_t ip = 0x04030201; /* 1.2.3.4 */

    printf("ip & 0x000000ff %8u 0x%08X >> 0  %u\n", ip & 0x000000ff, ip & 0x000000ff, (ip & 0x000000ff) >> 0);
    printf("ip & 0x0000ff00 %8u 0x%08X >> 8  %u\n", ip & 0x0000ff00, ip & 0x0000ff00, (ip & 0x0000ff00) >> 8);
    printf("ip & 0x00ff0000 %8u 0x%08X >> 16 %u\n", ip & 0x00ff0000, ip & 0x00ff0000, (ip & 0x00ff0000) >> 16);
    printf("ip & 0xff000000 %8u 0x%08X >> 24 %u\n", ip & 0xff000000, ip & 0xff000000, (ip & 0xff000000) >> 24);

    return 0;
}

void SigTableSetup(void) {
    memset(sigmatch_table, 0, sizeof(sigmatch_table));

    DetectSidRegister();
    DetectPriorityRegister();
    DetectRevRegister();
    DetectClasstypeRegister();
    DetectReferenceRegister();
    DetectThresholdRegister();
    DetectMetadataRegister();
    DetectMsgRegister();
    DetectContentRegister();
    DetectUricontentRegister();
    DetectPcreRegister();
    DetectDepthRegister();
    DetectNocaseRegister();
    DetectRecursiveRegister();
    DetectRawbytesRegister();
    DetectWithinRegister();
    DetectDistanceRegister();
    DetectOffsetRegister();
    DetectFlowRegister();
    DetectDsizeRegister();
    DetectFlowvarRegister();
    DetectAddressRegister();
    DetectNoalertRegister();

    u_int8_t i = 0;
    for (i = 0; i < DETECT_TBLSIZE; i++) {
        if (sigmatch_table[i].RegisterTests == NULL) {
            printf("Warning: detection plugin %s has no unittest "
                   "registration function.\n", sigmatch_table[i].name);
        }
    }
}

void SigTableRegisterTests(void) {
    /* register the tests */
    u_int8_t i = 0;
    for (i = 0; i < DETECT_TBLSIZE; i++) {
        if (sigmatch_table[i].RegisterTests != NULL) {
            sigmatch_table[i].RegisterTests();
        }
    }
}

/*
 * TESTS
 */

#include "flow-util.h"

int SigTest01 (void) {
    u_int8_t *buf = (u_int8_t *)
                    "GET /one/ HTTP/1.1\r\n"
                    "Host: one.example.org\r\n"
                    "\r\n\r\n"
                    "GET /two/ HTTP/1.1\r\n"
                    "Host: two.example.org\r\n"
                    "\r\n\r\n";
    u_int16_t buflen = strlen((char *)buf);
    Packet p;
    ThreadVars th_v;
    PatternMatcherThread *pmt;
    int result = 0;

    memset(&th_v, 0, sizeof(th_v));
    memset(&p, 0, sizeof(p));
    p.src.family = AF_INET;
    p.dst.family = AF_INET;
    p.tcp_payload = buf;
    p.tcp_payload_len = buflen;

    sig_list = SigInit("alert tcp any any -> any any (msg:\"HTTP URI cap\"; content:\"GET \"; depth:4; pcre:\"/GET (?P<http_uri>.*) HTTP\\/\\d\\.\\d\\r\\n/G\"; recursive; sid:1;)");
    if (sig_list == NULL) {
        result = 0;
        goto end;
    }

    SigGroupBuild(sig_list);
    PatternMatchPrepare(mpm_ctx);
    PatternMatcherThreadInit(&th_v, (void *)&pmt);

    SigMatchSignatures(&th_v, pmt, &p);
    if (PacketAlertCheck(&p, 1) == 0) {
        result = 0;
        goto end;
    }

    //printf("URI0 \"%s\", len %u\n", p.http_uri.raw[0], p.http_uri.raw_size[0]);
    //printf("URI1 \"%s\", len %u\n", p.http_uri.raw[1], p.http_uri.raw_size[1]);

    if (p.http_uri.raw_size[0] == 5 &&
        memcmp(p.http_uri.raw[0], "/one/", 5) == 0 &&
        p.http_uri.raw_size[1] == 5 &&
        memcmp(p.http_uri.raw[1], "/two/", 5) == 0)
    {
        result = 1;
    }

    SigGroupCleanup();
    PatternMatcherThreadDeinit(&th_v, (void *)pmt);
    PatternMatchDestroy(mpm_ctx);
    SigCleanSignatures();
end:
    return result;
}

int SigTest02 (void) {
    u_int8_t *buf = (u_int8_t *)
                    "GET /one/ HTTP/1.1\r\n"
                    "Host: one.example.org\r\n"
                    "\r\n\r\n"
                    "GET /two/ HTTP/1.1\r\n"
                    "Host: two.example.org\r\n"
                    "\r\n\r\n";
    u_int16_t buflen = strlen((char *)buf);
    Packet p;
    ThreadVars th_v;
    PatternMatcherThread *pmt;
    int result = 0;

    memset(&th_v, 0, sizeof(th_v));
    memset(&p, 0, sizeof(p));
    p.src.family = AF_INET;
    p.dst.family = AF_INET;
    p.tcp_payload = buf;
    p.tcp_payload_len = buflen;

    sig_list = SigInit("alert tcp any any -> any any (msg:\"HTTP TEST\"; content:\"Host: one.example.org\"; offset:20; depth:41; sid:1;)");
    if (sig_list == NULL) {
        result = 0;
        goto end;
    }

    SigGroupBuild(sig_list);
    PatternMatchPrepare(mpm_ctx);
    PatternMatcherThreadInit(&th_v, (void *)&pmt);

    SigMatchSignatures(&th_v, pmt, &p);
    if (PacketAlertCheck(&p, 1))
        result = 1;

    SigGroupCleanup();
    PatternMatcherThreadDeinit(&th_v, (void *)pmt);
    PatternMatchDestroy(mpm_ctx);
    SigCleanSignatures();
end:
    return result;
}

int SigTest03 (void) {
    u_int8_t *buf = (u_int8_t *)
                    "GET /one/ HTTP/1.1\r\n"
                    "Host: one.example.org\r\n"
                    "\r\n\r\n"
                    "GET /two/ HTTP/1.1\r\n"
                    "Host: two.example.org\r\n"
                    "\r\n\r\n";
    u_int16_t buflen = strlen((char *)buf);
    Packet p;
    ThreadVars th_v;
    PatternMatcherThread *pmt;
    int result = 0;

    memset(&th_v, 0, sizeof(th_v));
    memset(&p, 0, sizeof(p));
    p.src.family = AF_INET;
    p.dst.family = AF_INET;
    p.tcp_payload = buf;
    p.tcp_payload_len = buflen;

    sig_list = SigInit("alert tcp any any -> any any (msg:\"HTTP TEST\"; content:\"Host: one.example.org\"; offset:20; depth:40; sid:1;)");
    if (sig_list == NULL) {
        result = 0;
        goto end;
    }

    SigGroupBuild(sig_list);

    PatternMatchPrepare(mpm_ctx);
    PatternMatcherThreadInit(&th_v, (void *)&pmt);
    SigMatchSignatures(&th_v, pmt, &p);

    if (!PacketAlertCheck(&p, 1))
        result = 1;

    SigGroupCleanup();
    PatternMatcherThreadDeinit(&th_v, (void *)pmt);
    PatternMatchDestroy(mpm_ctx);
    SigCleanSignatures();
end:
    return result;
}

int SigTest04 (void) {
    u_int8_t *buf = (u_int8_t *)
                    "GET /one/ HTTP/1.1\r\n"
                    "Host: one.example.org\r\n"
                    "\r\n\r\n"
                    "GET /two/ HTTP/1.1\r\n"
                    "Host: two.example.org\r\n"
                    "\r\n\r\n";
    u_int16_t buflen = strlen((char *)buf);

    Packet p;
    ThreadVars th_v;
    PatternMatcherThread *pmt;
    int result = 0;

    memset(&th_v, 0, sizeof(th_v));
    memset(&p, 0, sizeof(p));
    p.src.family = AF_INET;
    p.dst.family = AF_INET;
    p.tcp_payload = buf;
    p.tcp_payload_len = buflen;

    sig_list = SigInit("alert tcp any any -> any any (msg:\"HTTP TEST\"; content:\"Host:\"; offset:20; depth:25; content:\"Host:\"; distance:47; within:52; sid:1;)");
    if (sig_list == NULL) {
        result = 0;
        goto end;
    }

    SigGroupBuild(sig_list);
    PatternMatchPrepare(mpm_ctx);
    PatternMatcherThreadInit(&th_v, (void *)&pmt);
    SigMatchSignatures(&th_v, pmt, &p);

    if (PacketAlertCheck(&p, 1))
        result = 1;

    SigGroupCleanup();
    PatternMatcherThreadDeinit(&th_v, (void *)pmt);
    PatternMatchDestroy(mpm_ctx);
    SigCleanSignatures();
end:
    return result;
}

int SigTest05 (void) {
    u_int8_t *buf = (u_int8_t *)
                    "GET /one/ HTTP/1.1\r\n"    /* 20 */
                    "Host: one.example.org\r\n" /* 23, 43 */
                    "\r\n\r\n"                  /* 4,  47 */
                    "GET /two/ HTTP/1.1\r\n"    /* 20, 67 */
                    "Host: two.example.org\r\n" /* 23, 90 */
                    "\r\n\r\n";                 /* 4,  94 */
    u_int16_t buflen = strlen((char *)buf);
    Packet p;
    ThreadVars th_v;
    PatternMatcherThread *pmt;
    int result = 0;

    memset(&th_v, 0, sizeof(th_v));
    memset(&p, 0, sizeof(p));
    p.src.family = AF_INET;
    p.dst.family = AF_INET;
    p.tcp_payload = buf;
    p.tcp_payload_len = buflen;

    sig_list = SigInit("alert tcp any any -> any any (msg:\"HTTP TEST\"; content:\"Host:\"; offset:20; depth:25; content:\"Host:\"; distance:48; within:52; sid:1;)");
    if (sig_list == NULL) {
        result = 0;
        goto end;
    }

    SigGroupBuild(sig_list);
    PatternMatchPrepare(mpm_ctx);
    PatternMatcherThreadInit(&th_v, (void *)&pmt);
    SigMatchSignatures(&th_v, pmt, &p);

    if (!PacketAlertCheck(&p, 1))
        result = 1;

    SigGroupCleanup();
    PatternMatcherThreadDeinit(&th_v, (void *)pmt);
    PatternMatchDestroy(mpm_ctx);
    SigCleanSignatures();
end:
    return result;
}

int SigTest06 (void) {
    u_int8_t *buf = (u_int8_t *)
                    "GET /one/ HTTP/1.1\r\n"    /* 20 */
                    "Host: one.example.org\r\n" /* 23, 43 */
                    "\r\n\r\n"                  /* 4,  47 */
                    "GET /two/ HTTP/1.1\r\n"    /* 20, 67 */
                    "Host: two.example.org\r\n" /* 23, 90 */
                    "\r\n\r\n";                 /* 4,  94 */
    u_int16_t buflen = strlen((char *)buf);
    Packet p;
    ThreadVars th_v;
    PatternMatcherThread *pmt;
    int result = 0;

    memset(&th_v, 0, sizeof(th_v));
    memset(&p, 0, sizeof(p));
    p.src.family = AF_INET;
    p.dst.family = AF_INET;
    p.tcp_payload = buf;
    p.tcp_payload_len = buflen;

    sig_list = SigInit("alert tcp any any -> any any (msg:\"HTTP URI cap\"; content:\"GET \"; depth:4; pcre:\"/GET (?P<http_uri>.*) HTTP\\/\\d\\.\\d\\r\\n/G\"; recursive; sid:1;)");
    if (sig_list == NULL) {
        result = 0;
        goto end;
    }
    sig_list->next = SigInit("alert tcp any any -> any any (msg:\"HTTP URI test\"; uricontent:\"two\"; sid:2;)");
    if (sig_list->next == NULL) {
        result = 0;
        goto end;
    }

    SigGroupBuild(sig_list);
    PatternMatchPrepare(mpm_ctx);
    PatternMatcherThreadInit(&th_v, (void *)&pmt);
    SigMatchSignatures(&th_v, pmt, &p);

    if (PacketAlertCheck(&p, 1) && PacketAlertCheck(&p, 2))
        result = 1;

    SigGroupCleanup();
    PatternMatcherThreadDeinit(&th_v, (void *)pmt);
    PatternMatchDestroy(mpm_ctx);
    SigCleanSignatures();
end:
    return result;
}

int SigTest07 (void) {
    u_int8_t *buf = (u_int8_t *)
                    "GET /one/ HTTP/1.1\r\n"    /* 20 */
                    "Host: one.example.org\r\n" /* 23, 43 */
                    "\r\n\r\n"                  /* 4,  47 */
                    "GET /two/ HTTP/1.1\r\n"    /* 20, 67 */
                    "Host: two.example.org\r\n" /* 23, 90 */
                    "\r\n\r\n";                 /* 4,  94 */
    u_int16_t buflen = strlen((char *)buf);
    Packet p;
    ThreadVars th_v;
    PatternMatcherThread *pmt;
    int result = 0;

    memset(&th_v, 0, sizeof(th_v));
    memset(&p, 0, sizeof(p));
    p.src.family = AF_INET;
    p.dst.family = AF_INET;
    p.tcp_payload = buf;
    p.tcp_payload_len = buflen;

    sig_list = SigInit("alert tcp any any -> any any (msg:\"HTTP URI cap\"; content:\"GET \"; depth:4; pcre:\"/GET (?P<http_uri>.*) HTTP\\/\\d\\.\\d\\r\\n/G\"; recursive; sid:1;)");
    if (sig_list == NULL) {
        result = 0;
        goto end;
    }
    sig_list->next = SigInit("alert tcp any any -> any any (msg:\"HTTP URI test\"; uricontent:\"three\"; sid:2;)");
    if (sig_list->next == NULL) {
        result = 0;
        goto end;
    }

    SigGroupBuild(sig_list);
    PatternMatchPrepare(mpm_ctx);
    PatternMatcherThreadInit(&th_v, (void *)&pmt);
    SigMatchSignatures(&th_v, pmt, &p);

    if (PacketAlertCheck(&p, 1) && PacketAlertCheck(&p, 2))
        result = 0;
    else
        result = 1;

    SigGroupCleanup();
    PatternMatcherThreadDeinit(&th_v, (void *)pmt);
    PatternMatchDestroy(mpm_ctx);
    SigCleanSignatures();
end:
    return result;
}

int SigTest08 (void) {
    u_int8_t *buf = (u_int8_t *)
                    "GET /one/ HTTP/1.0\r\n"    /* 20 */
                    "Host: one.example.org\r\n" /* 23, 43 */
                    "\r\n\r\n"                  /* 4,  47 */
                    "GET /two/ HTTP/1.0\r\n"    /* 20, 67 */
                    "Host: two.example.org\r\n" /* 23, 90 */
                    "\r\n\r\n";                 /* 4,  94 */
    u_int16_t buflen = strlen((char *)buf);
    Packet p;
    ThreadVars th_v;
    PatternMatcherThread *pmt;
    int result = 0;

    memset(&th_v, 0, sizeof(th_v));
    memset(&p, 0, sizeof(p));
    p.src.family = AF_INET;
    p.dst.family = AF_INET;
    p.tcp_payload = buf;
    p.tcp_payload_len = buflen;

    sig_list = SigInit("alert tcp any any -> any any (msg:\"HTTP URI cap\"; content:\"GET \"; depth:4; pcre:\"/GET (?P<http_uri>.*) HTTP\\/1\\.0\\r\\n/G\"; sid:1;)");
    if (sig_list == NULL) {
        result = 0;
        goto end;
    }
    sig_list->next = SigInit("alert tcp any any -> any any (msg:\"HTTP URI test\"; uricontent:\"one\"; sid:2;)");
    if (sig_list->next == NULL) {
        result = 0;
        goto end;
    }

    SigGroupBuild(sig_list);
    PatternMatchPrepare(mpm_ctx);
    PatternMatcherThreadInit(&th_v, (void *)&pmt);
    SigMatchSignatures(&th_v, pmt, &p);

    if (PacketAlertCheck(&p, 1) && PacketAlertCheck(&p, 2))
        result = 1;

    SigGroupCleanup();
    PatternMatcherThreadDeinit(&th_v, (void *)pmt);
    PatternMatchDestroy(mpm_ctx);
    SigCleanSignatures();
end:
    return result;
}

int SigTest09 (void) {
    u_int8_t *buf = (u_int8_t *)
                    "GET /one/ HTTP/1.0\r\n"    /* 20 */
                    "Host: one.example.org\r\n" /* 23, 43 */
                    "\r\n\r\n"                  /* 4,  47 */
                    "GET /two/ HTTP/1.0\r\n"    /* 20, 67 */
                    "Host: two.example.org\r\n" /* 23, 90 */
                    "\r\n\r\n";                 /* 4,  94 */
    u_int16_t buflen = strlen((char *)buf);
    Packet p;
    ThreadVars th_v;
    PatternMatcherThread *pmt;
    int result = 0;

    memset(&th_v, 0, sizeof(th_v));
    memset(&p, 0, sizeof(p));
    p.src.family = AF_INET;
    p.dst.family = AF_INET;
    p.tcp_payload = buf;
    p.tcp_payload_len = buflen;

    sig_list = SigInit("alert tcp any any -> any any (msg:\"HTTP URI cap\"; content:\"GET \"; depth:4; pcre:\"/GET (?P<http_uri>.*) HTTP\\/1\\.0\\r\\n/G\"; sid:1;)");
    if (sig_list == NULL) {
        result = 0;
        goto end;
    }
    sig_list->next = SigInit("alert tcp any any -> any any (msg:\"HTTP URI test\"; uricontent:\"two\"; sid:2;)");
    if (sig_list->next == NULL) {
        result = 0;
        goto end;
    }

    SigGroupBuild(sig_list);
    PatternMatchPrepare(mpm_ctx);
    PatternMatcherThreadInit(&th_v, (void *)&pmt);
    SigMatchSignatures(&th_v, pmt, &p);

    if (PacketAlertCheck(&p, 1) && PacketAlertCheck(&p, 2))
        result = 0;
    else
        result = 1;

    SigGroupCleanup();
    PatternMatcherThreadDeinit(&th_v, (void *)pmt);
    PatternMatchDestroy(mpm_ctx);
    SigCleanSignatures();
end:
    return result;
}

void SigRegisterTests(void) {
    SigParseRegisterTests();
    UtRegisterTest("SigTest01 -- HTTP URI cap", SigTest01, 1);
    UtRegisterTest("SigTest02 -- Offset/Depth match", SigTest02, 1);
    UtRegisterTest("SigTest03 -- offset/depth mismatch", SigTest03, 1);
    UtRegisterTest("SigTest04 -- distance/within match", SigTest04, 1);
    UtRegisterTest("SigTest05 -- distance/within mismatch", SigTest05, 1);
    UtRegisterTest("SigTest06 -- uricontent HTTP/1.1 match test", SigTest06, 1);
    UtRegisterTest("SigTest07 -- uricontent HTTP/1.1 mismatch test", SigTest07, 1);
    UtRegisterTest("SigTest08 -- uricontent HTTP/1.0 match test", SigTest08, 1);
    UtRegisterTest("SigTest09 -- uricontent HTTP/1.0 mismatch test", SigTest09, 1);
}

