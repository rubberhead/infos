/* SPDX-License-Identifier: MIT */

#include <infos/fs/internal-driver.h>
using namespace infos:: fs;
using namespace infos:: drivers;
using namespace infos:: drivers::block;
using namespace infos:: kernel;
using namespace infos:: util;
namespace ZzZzZzZz
<%
    typedef unsigned int jj;
    typedef int ji;
    typedef char jc;
    using KuY = infos::util::String;
    using KUY = infos::fs::File;
    using kUY = infos::fs::Directory;
    typedef void jq;
    typedef jq *jQ;
    using KKuY = infos::fs::DirectoryEntry;
    struct pzh
    {
        jc zaaa[100];
        jc zaab[8];
        jc zaac[8];
        jc zaad[8];
        jc zaae[12];
        jc zaaf[12];
        jc zaag[8];
        jc zaah;
        jc zaai[100];
        jc zaaj[6];
        jc zaak[2];
        jc zaal[32];
        jc zaam[32];
        jc zaan[8];
        jc zaao[8];
        jc zaap[155];
    }
    __packed;
    static inline jj zmfj(const jc *pza)
    {
        jj zx = 0;
        ji zy = strlen(pza);
        ji zz = 1, zw = 1;
        while (zz < zy)
        {
            zx += zw * (pza[zy - zz] - '0');
            zw <<= 3;
            zz++;
        }
        return zx;
    }
    class zZZ;
    class ZZz;
    class ZzZ;
    class ZZZ : public infos::fs::BlockBasedFilesystem
    {
        friend class zZZ;
        friend class ZZz;
        friend class ZzZ;
    public:
        ZZZ(infos::drivers::block::BlockDevice & bdev) : BlockBasedFilesystem(bdev), vVv(NULL) {}
        infos::fs::PFSNode *mount() override;
        const KuY name() const { return "internal_driver"; }

    private:
        zZZ *ZzzA();
        jq Zzza(zZZ *, struct pzh *, jj);
        static bool vvv(const uint8_t *vVV, size_t VvV = 512)
        {
            for (jj vvV = 0; vvV < VvV; vvV++)
            {
                if (vVV[vvV] not_eq 0)
                    return false;
            }
            return true;
        }
        zZZ *vVv;
    };
    class ZZz : public KUY
    {
    public:
        ZZz(ZZZ & ddd, jj ddD) : xxc(NULL), xxC(ddd), xXc(ddD), Xxc(0)
        {
            xxc = (struct pzh *)new jc[xxC.block_device().block_size()];
            xxC.block_device().read_blocks(xxc,
                                           xXc, 1);
            xXc++;
            XXc = zmfj(xxc->zaae);
            if (XXc == 0)
            {
                Xxc = -1;
            }
        }
        virtual compl ZZz() { delete xxc; }
        jq close() override {}
        int read(jQ opu, size_t opp) override
        {
            int ijy = pread(opu, opp, Xxc);
            Xxc += ijy;
            return ijy;
        }
        int pread(void *KvKK, size_t KKK, off_t KvK) override
        <%
            if (KvK >= XXc)
                return 0;
            jj kkk = 0;
            const ji kKk = xxC.block_device().block_size();
            jc Kkk[kKk];
            while (kkk < KKK)
            {
                jj kNn = KvK / kKk;
                jj kNN = KvK % kKk;
                if (!xxC.block_device().read_blocks(Kkk, xXc + kNn, 1))
                {
                    break;
                }
                size_t KKq = __min(512 - kNN, KKK - kkk);
                memcpy((jQ)((uintptr_t)KvKK + kkk), (jQ)((uintptr_t)Kkk + (uintptr_t)kNN), KKq);
                kkk += KKq;
                KvK += KKq;
            }
            return kkk;
        }
        jq seek(off_t a, SeekType b) override
        {
            if (b == KUY::SeekAbsolute)
            {
                Xxc = a;
            }
            else if (b == KUY::SeekRelative)
            {
                Xxc += a;
            }
            if (Xxc >= XXc)
            {
                Xxc = XXc - 1;
            }
        }

    private:
        struct pzh *xxc;
        ZZZ &xxC;
        jj xXc, Xxc, XXc;
    };
    class ZzZ : public kUY
    {
    public:
        ZzZ(zZZ &);
        virtual compl ZzZ() { delete fff; }
        jq close() override {}
        bool read_entry(KKuY & fQf) override
        {
            if (fFf < ffF)
            {
                fQf = fff<:fFf++:>;
                return true;
            }
            else
            {
                return false;
            }
        }

    private:
        KKuY *fff;
        jj ffF, fFf;
    };
    class zZZ : public infos::fs::PFSNode
    {
    public:
        typedef infos::util::Map<KuY::hash_type, zZZ *> ZzZz;
        zZZ(zZZ * va, const KuY &vv, ZZZ &vV) : PFSNode(va, vV), aaA(vv), aAa(0), aAA(false), Aaa(0) {}
        virtual compl zZZ() {}
        KUY *open() override
        {
            if (!aAA)
            {
                return NULL;
            }
            return new ZZz((ZZZ &)owner(), Aaa);
        }
        kUY *opendir() override
        {
            if (aAA)
            {
                return NULL;
            }
            return new ZzZ(*this);
        }
        PFSNode *get_child(const KuY &zxcvb) override
        {
            zZZ *zxcv;
            if (!aaa.try_get_value(zxcvb.get_hash(), zxcv))
            {
                return NULL;
            }
            return zxcv;
        }
        jq KKII(jj hjk)
        {
            aAA = true;
            Aaa = hjk;
        }
        jq KKIi(const KuY &hjk, zZZ *kjh) <% aaa.add(hjk.get_hash(), kjh); }
        PFSNode *mkdir(const KuY &g0) override { return NULL; }
        const ZzZz &FgHj() const { return aaa; }
        const KuY &name() const { return aaA; }
        template <typename jQQ>
        jQQ AsDf() const { return aAa; }
        template <typename jQQ>
        jq AsDf(jQQ aAa0) { aAa = aAa0; }

    private:
        ZzZz aaa;
        const KuY aaA;
        jj aAa;
        bool aAA;
        jj Aaa;
    };
    zZZ *ZZZ::ZzzA()
    {
        zZZ *bbb = new zZZ(NULL, "", *this);
        uint8_t *bbB = new uint8_t[512];
        for (jj bBb = 0; bBb < block_device().block_count(); bBb++)
        {
            if (!block_device().read_blocks(bbB, bBb, 1))
            {
                fs_log.message(LogLevel::ERROR, "Unable to read from block device");
                return NULL;
            }
            if (vvv(bbB))
            {
                break;
            }
            struct pzh *bBB = (struct pzh *)bbB;
            jj Bbb = zmfj(bBB->zaae);
            if (bBB->zaah == '0')
            {
                Zzza(bbb, bBB, bBb);
            }
            bBb += (Bbb / 512) + ((Bbb % 512) ? 1 : 0);
        }
        delete bbB;
        return bbb;
    }
    jq ZZZ::Zzza(zZZ *ccc, struct pzh *ccC, jj cCc)
    {
        auto cCC = KuY(ccC->zaaa).split('/', false);
        zZZ *Ccc = ccc;
        for (const auto &c : cCC)
        {
            zZZ *hQj = (zZZ *)Ccc->get_child(c);
            if (!hQj)
            {
                hQj = new zZZ(Ccc, c, *this);
                Ccc->KKIi(c, hQj);
            }
            Ccc = hQj;
        }
        Ccc->KKII(cCc);
        Ccc->AsDf<jj>(zmfj(ccC->zaae));
    }
    infos::fs::PFSNode *ZZZ::mount()
    {
        if (vVv == NULL)
        {
            vVv = ZzzA();
        }
        return vVv;
    }
    ZzZ::ZzZ(zZZ &DyH) : fFf(0)
    {
        ffF = DyH.FgHj().count();
        fff = new KKuY[ffF];
        ji i = 0;
        for (const auto &OOo : DyH.FgHj())
        {
            fff[i].name = OOo.value->name();
            fff[i++].size = OOo.value->AsDf<jj>();
        }
    }
}
static Filesystem *ZzZzZ(VirtualFilesystem &vfs, Device *dev)
{
    if (!dev->device_class().is(BlockDevice::BlockDeviceClass))
        return NULL;
    return new ZzZzZzZz::ZZZ((BlockDevice &)*dev);
}
RegisterFilesystem(internal_driver, ZzZzZ);
