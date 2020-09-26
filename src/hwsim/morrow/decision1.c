
void
decision_1()
{
	create_multio(PORT_MULTIO_MASTER);
	create_djdma(PORT_DJDMA, vi_1);
	create_hdca(PORT_HDCA, vi_0);
	create_hdc_dma(PORT_HDCDMA, vi_0);
}

