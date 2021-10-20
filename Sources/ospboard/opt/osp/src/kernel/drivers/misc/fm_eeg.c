#include <linux/version.h>
#include <linux/of.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/spi/spi.h>
#include <linux/interrupt.h>

MODULE_AUTHOR("Sean Hamilton <skhamilt@eng.ucsd.edu>");
MODULE_LICENSE("GPL");

/*
 * The structure to represent 'fmeeg_dev' devices.
 *  data - data buffer;
 *  lock - a mutex to protect the fields of this structure;
 */
struct fmeeg_dev {
	unsigned char *data;
	struct spi_device *spi;
	struct mutex lock;
};

static const struct spi_device_id fmeeg_spi_ids[] = {
	{"fmeeg", 0},
	{},
};
MODULE_DEVICE_TABLE(spi, fmeeg_spi_ids);

static const struct of_device_id fmeeg_of_ids[] = {
	{ .compatible = "osp,fmeeg", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of,fmeeg_of_ids);

#if 0
static u8 samples[1];
static int fmeeg_read(struct device *dev)
{
	struct spi_transfer xfer = {
		.rx_buf         = samples,
		.len            = 1,
		.bits_per_word  = 8,
	};
	struct spi_message msg;
	struct spi_device *spi = to_spi_device(dev);
	int error;

	spi_message_init(&msg);
	spi_message_add_tail(&xfer, &msg);

	error = spi_sync(spi, &msg);
	if (error) {
		dev_err(dev, "%s: failed read, spi error: %d\n", __func__, error);
		return error;
	}

	return 0;
}
#endif

static irqreturn_t fmeeg_irq_handler(int cpl, void *dev)
{
	/* start a read transfer */
	return 0;
}

static int fmeeg_probe(struct spi_device *spi)
{
	int ret = 0;
	struct fmeeg_dev *eeg;
#if 0
	const struct of_device_id *match;

	match = of_match_device(of_match_ptr(fmeeg_of_ids), &spi->dev);
	if (match) {
		/* DT code here */
	} else {
		/* Platform data code here */
	}
#endif

	eeg = devm_kzalloc(&spi->dev, sizeof(struct fmeeg_dev), GFP_KERNEL);
	if (!eeg) {
		return -ENOMEM;
	}

	mutex_init(&eeg->lock);
	spi_set_drvdata(spi, eeg);

	eeg->spi = spi;
	eeg->spi->mode = SPI_MODE_0;
	eeg->spi->max_speed_hz = 2000000;

	ret = spi_setup(eeg->spi);
	if (ret < 0) {
		return ret;
	}

	if (request_irq(38, fmeeg_irq_handler, IRQF_SHARED, "fmeeg", eeg)) {
		printk(KERN_ERR "my_device: cannot register IRQ %d\n", 38);
		return -EIO;
	}

	/* more setup here */

	return ret;
}

static int fmeeg_remove(struct spi_device *spi)
{
	struct fmeeg_dev *eeg;

	eeg = spi_get_drvdata(spi);
	if (!eeg) {
		return -ENODEV;
	}

	mutex_destroy(&eeg->lock);

	return 0;
}

static struct spi_driver fmeeg_spi_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "fmeeg",
		.of_match_table = of_match_ptr(fmeeg_of_ids),
	},
	.probe = fmeeg_probe,
	.remove = fmeeg_remove,
	.id_table = fmeeg_spi_ids,
};
module_spi_driver(fmeeg_spi_driver);
